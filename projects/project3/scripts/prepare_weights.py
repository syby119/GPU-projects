#!/usr/bin/env python3
"""Convert the official torchvision AlexNet checkpoint using only Python's stdlib."""

import argparse
from collections import OrderedDict
from dataclasses import dataclass
import hashlib
import io
import math
from pathlib import Path
import pickle
import struct
import urllib.request
import zipfile


URL = "https://download.pytorch.org/models/alexnet-owt-7be5be79.pth"
SHA256 = "7be5be791159472b1fbf3c69796f7cb30dca7ad8466c2df70058c37116cdee02"
LAYERS = [
    ("features.0", (64, 3, 11, 11)),
    ("features.3", (192, 64, 5, 5)),
    ("features.6", (384, 192, 3, 3)),
    ("features.8", (256, 384, 3, 3)),
    ("features.10", (256, 256, 3, 3)),
    ("classifier.1", (4096, 9216)),
    ("classifier.4", (4096, 4096)),
    ("classifier.6", (1000, 4096)),
]


class FloatStorage:
    """Marker for a storage description; no torch module is imported."""


@dataclass
class Storage:
    key: str
    count: int


@dataclass
class Tensor:
    storage: Storage
    offset: int
    shape: tuple
    stride: tuple


def rebuild_tensor(storage, offset, shape, stride, requires_grad, hooks):
    return Tensor(storage, offset, shape, stride)


def rebuild_parameter(tensor, requires_grad, hooks):
    return tensor


class CheckpointMetadata(pickle.Unpickler):
    """Only recognize the four globals present in this exact checkpoint."""

    def find_class(self, module, name):
        allowed = {
            ("collections", "OrderedDict"): OrderedDict,
            ("torch", "FloatStorage"): FloatStorage,
            ("torch._utils", "_rebuild_tensor_v2"): rebuild_tensor,
            ("torch._utils", "_rebuild_parameter"): rebuild_parameter,
        }
        try:
            return allowed[(module, name)]
        except KeyError:
            raise ValueError(
                f"Unsupported checkpoint global: {module}.{name}"
            ) from None

    def persistent_load(self, identifier):
        if (
            not isinstance(identifier, tuple)
            or len(identifier) != 5
            or identifier[0] != "storage"
            or identifier[1] is not FloatStorage
            or identifier[3] != "cpu"
            or not isinstance(identifier[2], str)
            or not identifier[2].isdigit()
            or not isinstance(identifier[4], int)
            or identifier[4] < 0
        ):
            raise ValueError("Unsupported storage description")
        return Storage(identifier[2], identifier[4])


def file_hash(path):
    with path.open("rb") as source:
        return hashlib.file_digest(source, "sha256").hexdigest()


def download_checkpoint(path):
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix(path.suffix + ".part")
    try:
        print(f"Downloading {URL}", flush=True)
        with (
            urllib.request.urlopen(URL, timeout=60) as source,
            temporary.open("wb") as output,
        ):
            while chunk := source.read(1024 * 1024):
                output.write(chunk)
        if file_hash(temporary) != SHA256:
            raise ValueError("Downloaded checkpoint SHA256 does not match")
        temporary.replace(path)
    finally:
        temporary.unlink(missing_ok=True)


def convert(checkpoint, destination):
    # Pinning the hash makes this a converter for one known archive, rather
    # than an attempted implementation of the general torch.load interface.
    if file_hash(checkpoint) != SHA256:
        raise ValueError("Expected the official alexnet-owt-7be5be79.pth checkpoint")

    destination.parent.mkdir(parents=True, exist_ok=True)
    temporary = destination.with_suffix(destination.suffix + ".part")

    try:
        with zipfile.ZipFile(checkpoint) as archive:
            metadata = archive.read("archive/data.pkl")
            if len(metadata) > 65536:
                raise ValueError("Unexpected checkpoint metadata size")
            tensors = CheckpointMetadata(io.BytesIO(metadata)).load()

            expected = [
                (name + suffix, shape if suffix == ".weight" else (shape[0],))
                for name, shape in LAYERS
                for suffix in (".weight", ".bias")
            ]
            if set(tensors) != {name for name, shape in expected}:
                raise ValueError("Checkpoint has missing or unexpected tensors")

            with temporary.open("wb") as output:
                output.write(b"ALEXNET1")
                output.write(struct.pack("<II", 1, len(expected)))

                for name, shape in expected:
                    tensor = tensors[name]
                    stride = tuple(math.prod(shape[i + 1 :]) for i in range(len(shape)))
                    count = math.prod(shape)

                    if (
                        not isinstance(tensor, Tensor)
                        or tensor.shape != shape
                        or tensor.stride != stride
                        or tensor.offset != 0
                        or tensor.storage.count != count
                    ):
                        raise ValueError(
                            f"Unexpected shape, stride, or storage for {name}"
                        )

                    data = archive.read("archive/data/" + tensor.storage.key)
                    if len(data) != count * 4:
                        raise ValueError(f"Wrong data size for {name}")

                    encoded = name.encode("ascii")
                    output.write(struct.pack("<I", len(encoded)))
                    output.write(encoded)
                    output.write(
                        struct.pack("<II", 1, len(shape))
                    )  # dtype=1: little-endian FP32
                    output.write(struct.pack("<" + "I" * len(shape), *shape))
                    output.write(struct.pack("<Q", len(data)))
                    output.write(data)

                    print(
                        f"{name:24s} {str(shape):24s} {len(data):>10,} bytes",
                        flush=True,
                    )

        temporary.replace(destination)
    finally:
        temporary.unlink(missing_ok=True)

    print(f"Wrote {destination} ({destination.stat().st_size:,} bytes)")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--checkpoint",
        type=Path,
        default=Path("out/project3-data/alexnet-owt-7be5be79.pth"),
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=Path(__file__).resolve().parents[1] / "assets/models/alexnet.bin",
    )
    parser.add_argument(
        "--download",
        action="store_true",
        help="Download the checkpoint if it is missing",
    )
    options = parser.parse_args()

    if options.checkpoint.resolve() == options.output.resolve():
        parser.error("--output must differ from --checkpoint")
    try:
        if not options.checkpoint.exists() and options.download:
            download_checkpoint(options.checkpoint)
        convert(options.checkpoint, options.output)
    except (OSError, ValueError, pickle.UnpicklingError, zipfile.BadZipFile) as error:
        parser.exit(1, f"prepare_weights: {error}\n")


if __name__ == "__main__":
    main()
