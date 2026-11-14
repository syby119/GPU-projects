#!/usr/bin/env python3
"""Prepare real labeled images for CPU/GPU validation.

Only this offline preparation tool needs Pillow.
Weight conversion and the C++/CUDA program remain independent of Pillow.
"""

import argparse
import hashlib
import io
import json
from pathlib import Path, PurePosixPath
import random
import tarfile
import urllib.request


IMAGENETTE_URL = "https://s3.amazonaws.com/fast-ai-imageclas/imagenette2-320.tgz"
IMAGENETTE_BYTES = 341663724
CLASS_INDEX_URL = "https://storage.googleapis.com/download.tensorflow.org/data/imagenet_class_index.json"
CLASS_INDEX_SHA256 = "a1e7a966a1f601d39e4b43e119b3e7dd4a2ad3ea08cf69847cbaf021013767bc"


def sha256(path):
    with path.open("rb") as file:
        return hashlib.file_digest(file, "sha256").hexdigest()


def download(url, path):
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix(path.suffix + ".part")
    print(f"Downloading {url}", flush=True)
    try:
        with (
            urllib.request.urlopen(url, timeout=60) as source,
            temporary.open("wb") as output,
        ):
            while block := source.read(1024 * 1024):
                output.write(block)
        temporary.replace(path)
    finally:
        temporary.unlink(missing_ok=True)


def class_mapping(path):
    if not path.exists():
        download(CLASS_INDEX_URL, path)
    if sha256(path) != CLASS_INDEX_SHA256:
        raise ValueError("Unexpected ImageNet class-index mapping")
    mapping = json.loads(path.read_text())
    wnids = [mapping[str(index)][0] for index in range(1000)]
    if len(set(wnids)) != 1000 or wnids != sorted(wnids):
        raise ValueError(
            "Class mapping does not use canonical sorted ImageNet WNID order"
        )
    categories = [mapping[str(index)][1].replace("_", " ") for index in range(1000)]
    return {wnid: index for index, wnid in enumerate(wnids)}, categories


def choose_per_class(groups, count, seed):
    generator = random.Random(seed)
    selected = []
    for wnid in sorted(groups):
        candidates = sorted(groups[wnid])
        selected.extend(
            generator.sample(candidates, min(count, len(candidates)))
            if count
            else candidates
        )
    if not selected:
        raise ValueError("No labeled images found")
    return selected


def archive_images(path, mapping, count, seed):
    # Read only regular val/<wnid>/<image> entries. Labels come from dataset
    # annotations encoded in folder names, never from model predictions.
    # Imagenette reshuffled ImageNet data: keep original validation filenames
    # so this accuracy sample does not include pretrained training images.
    with tarfile.open(path, "r:gz") as archive:
        groups = {}
        entries = {}
        for member in archive.getmembers():
            parts = PurePosixPath(member.name).parts
            if (
                member.isfile()
                and len(parts) == 4
                and parts[0] == "imagenette2-320"
                and parts[1] == "val"
                and parts[2] in mapping
                and parts[3].startswith("ILSVRC2012_val_")
                and PurePosixPath(parts[3]).suffix.lower() in (".jpeg", ".jpg", ".png")
            ):
                groups.setdefault(parts[2], []).append(member.name)
                entries[member.name] = member
        if len(groups) != 10:
            raise ValueError(
                "Expected the ten clean Imagenette validation class folders"
            )
        selected = choose_per_class(groups, count, seed)
        # Forward offset order avoids repeatedly decompressing the same gzip data.
        for name in sorted(selected, key=lambda value: entries[value].offset_data):
            member = entries[name]
            with archive.extractfile(member) as image:
                yield name, mapping[PurePosixPath(name).parts[2]], image.read()


def folder_images(root, mapping, count, seed):
    groups = {}
    for directory in sorted(root.iterdir()):
        if directory.is_dir():
            if directory.name not in mapping:
                raise ValueError(f"Unknown ImageNet WNID folder: {directory.name}")
            groups[directory.name] = [
                str(path)
                for path in directory.rglob("*")
                if path.is_file()
                and path.suffix.lower() in (".jpeg", ".jpg", ".png", ".ppm")
            ]
    for name in choose_per_class(groups, count, seed):
        path = Path(name)
        wnid = path.relative_to(root).parts[0]
        yield path.relative_to(root).as_posix(), mapping[wnid], path.read_bytes()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument(
        "--dataset-root",
        type=Path,
        help="Validation folder containing ImageNet WNID subfolders",
    )
    source.add_argument(
        "--imagenette-archive", type=Path, help="Existing official imagenette2-320.tgz"
    )
    source.add_argument("--download-imagenette", action="store_true")
    parser.add_argument("--cache-dir", type=Path, default=Path("out/project3-data"))
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=Path(__file__).resolve().parents[1] / "assets/dataset",
    )
    parser.add_argument(
        "--per-class",
        type=int,
        default=10,
        help="Seeded sample count per class; 0 uses all images",
    )
    parser.add_argument("--seed", type=int, default=42)
    options = parser.parse_args()
    if options.per_class < 0 or options.seed < 0:
        parser.error("--per-class and --seed must be nonnegative")

    from PIL import Image, __version__ as pillow_version

    mapping, categories = class_mapping(options.cache_dir / "imagenet_class_index.json")

    archive = options.imagenette_archive
    if options.download_imagenette:
        archive = options.cache_dir / "imagenette2-320.tgz"
        if not archive.exists():
            download(IMAGENETTE_URL, archive)
    if archive:
        if archive.stat().st_size != IMAGENETTE_BYTES:
            raise ValueError("Unexpected Imagenette 320 archive size")
        images = archive_images(archive, mapping, options.per_class, options.seed)
        dataset = "Original ImageNet validation images retained in Imagenette val; ten-class subset, full 1000-way classifier"
    else:
        images = folder_images(
            options.dataset_root.resolve(), mapping, options.per_class, options.seed
        )
        dataset = "User-provided ImageNet WNID validation folders"

    destination = options.output_dir.resolve()
    (destination / "images").mkdir(parents=True, exist_ok=True)
    records = []

    for index, (source_name, target, image_bytes) in enumerate(images):
        with Image.open(io.BytesIO(image_bytes)) as original:
            image = original.convert("RGB")
            # The C++ program repeats crop/normalization/layout conversion
            # from these pre-resized RGB bytes in every inference pass.
            width, height = image.size
            if width <= height:
                size = (256, height * 256 // width)
            else:
                size = (width * 256 // height, 256)
            resized = image.resize(size, resample=Image.Resampling.BILINEAR)
            filename = f"images/{index:06d}.ppm"
            resized.save(destination / filename, format="PPM")

        records.append(
            {
                "image": filename,
                "source": source_name,
                "ground_truth": target,
                "ground_truth_name": categories[target],
            }
        )
        if (index + 1) % 10 == 0:
            print(f"Prepared {index + 1} labeled images", flush=True)

    count = len(records)
    if not count:
        raise ValueError("No evaluation images")
    (destination / "images.txt").write_text(
        "".join(record["image"] + "\n" for record in records), encoding="utf-8"
    )
    (destination / "labels.txt").write_text(
        "".join(str(record["ground_truth"]) + "\n" for record in records),
        encoding="utf-8",
    )
    (destination / "classes.txt").write_text(
        "\n".join(categories) + "\n", encoding="utf-8"
    )

    # Select up to ten evenly spaced images without duplicating image files.
    cpu_check_indices = list(range(0, count, max(1, count // 10)))[:10]
    (destination / "cpu-check-images.txt").write_text(
        "".join(records[index]["image"] + "\n" for index in cpu_check_indices),
        encoding="utf-8",
    )
    (destination / "cpu-check-labels.txt").write_text(
        "".join(
            str(records[index]["ground_truth"]) + "\n" for index in cpu_check_indices
        ),
        encoding="utf-8",
    )

    metadata = {
        "dataset": dataset,
        "source_url": IMAGENETTE_URL if archive else None,
        "archive_sha256": sha256(archive) if archive else None,
        "class_mapping_url": CLASS_INDEX_URL,
        "class_mapping_sha256": CLASS_INDEX_SHA256,
        "label_source": "Original dataset WNID folder annotations, mapped to ImageNet-1K [0,999]",
        "selection_rule": "Only val/ILSVRC2012_val_* original validation images"
        if archive
        else "Supplied validation WNID folders",
        "classes": 1000,
        "observed_classes": len({record["ground_truth"] for record in records}),
        "images": count,
        "per_class": options.per_class,
        "seed": options.seed,
        "cpu_check_indices": cpu_check_indices,
        "pillow": pillow_version,
        "preprocessing": "RGB, Pillow bilinear resize short side 256; C++ performs center crop 224 and normalization",
        "samples": records,
    }
    (destination / "evaluation.json").write_text(
        json.dumps(metadata, indent=2) + "\n", encoding="utf-8"
    )
    print(f"Wrote validation pack: {destination} ({count} images)")


if __name__ == "__main__":
    main()
