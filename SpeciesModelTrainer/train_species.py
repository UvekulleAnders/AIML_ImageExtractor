#!/usr/bin/env python3
"""Train a species classifier from AIML_ImageExtractor dataset exports."""

from __future__ import annotations

import argparse
import csv
import json
import random
import sys
from collections import defaultdict
from pathlib import Path

from PIL import Image, UnidentifiedImageError
import torch
from torch import nn
from torch.utils.data import DataLoader, Dataset
from torchvision import models, transforms


class DatasetError(RuntimeError):
    pass


class SpeciesImageDataset(Dataset):
    def __init__(self, rows, class_to_index, transform):
        self.rows = rows
        self.class_to_index = class_to_index
        self.transform = transform

    def __len__(self):
        return len(self.rows)

    def __getitem__(self, index):
        row = self.rows[index]
        try:
            with Image.open(row["path"]) as source:
                image = source.convert("RGB")
        except (OSError, UnidentifiedImageError) as error:
            raise DatasetError(f"Could not read image {row['path']}: {error}") from error
        return self.transform(image), self.class_to_index[row["class"]]


def read_export(dataset_root: Path):
    manifest_path = dataset_root / "manifest.csv"
    if not manifest_path.is_file():
        raise DatasetError(f"Could not find {manifest_path}")

    rows = []
    class_names = set()
    metadata = {}
    with manifest_path.open("r", encoding="utf-8-sig", newline="") as manifest:
        reader = csv.DictReader(manifest)
        required = {"file", "species_id", "swedish", "latin", "source_file"}
        if not reader.fieldnames or not required.issubset(reader.fieldnames):
            raise DatasetError("manifest.csv is missing one or more required columns: " + ", ".join(sorted(required)))

        for line_number, record in enumerate(reader, start=2):
            relative = Path(record["file"].replace("\\", "/"))
            image_path = (dataset_root / relative).resolve()
            try:
                image_path.relative_to(dataset_root.resolve())
            except ValueError as error:
                raise DatasetError(f"Unsafe image path in manifest line {line_number}") from error
            if not image_path.is_file():
                raise DatasetError(f"Image from manifest line {line_number} does not exist: {image_path}")

            class_name = image_path.parent.name
            class_names.add(class_name)
            metadata[class_name] = {
                "species_id": record["species_id"],
                "swedish": record["swedish"],
                "latin": record["latin"],
                "english": record.get("english", ""),
            }
            source = record.get("source_file", "").strip()
            group = source.casefold() if source else f"__image__:{image_path.as_posix().casefold()}"
            rows.append({"path": image_path, "class": class_name, "group": group})

    if not rows:
        raise DatasetError("manifest.csv contains no exported images")
    if len(class_names) < 2:
        raise DatasetError("At least two species classes are required")

    ordered_classes = sorted(class_names, key=str.casefold)
    return rows, ordered_classes, metadata


def split_by_source(rows, classes, validation_fraction, seed):
    groups_by_class = defaultdict(set)
    rows_by_group = defaultdict(list)
    for row in rows:
        groups_by_class[row["class"]].add(row["group"])
        rows_by_group[row["group"]].append(row)

    insufficient = [name for name in classes if len(groups_by_class[name]) < 2]
    if insufficient:
        names = ", ".join(insufficient)
        raise DatasetError(
            "A source-video-aware validation split requires at least two distinct source videos per species. "
            f"Insufficient classes: {names}. Add independent videos or provide more labeled data."
        )

    rng = random.Random(seed)
    source_groups = list(rows_by_group)
    rng.shuffle(source_groups)
    validation_groups = set()
    assigned_classes = set()
    for class_name in classes:
        if class_name in assigned_classes:
            continue
        choices = [group for group in source_groups if group not in validation_groups and any(r["class"] == class_name for r in rows_by_group[group])]
        if not choices:
            raise DatasetError(f"Could not create a validation group for species {class_name}")
        chosen = choices[0]
        validation_groups.add(chosen)
        assigned_classes.update(row["class"] for row in rows_by_group[chosen])

    target_count = max(len(validation_groups), round(len(source_groups) * validation_fraction))
    for group in source_groups:
        if len(validation_groups) >= target_count:
            break
        if group not in validation_groups:
            validation_groups.add(group)

    train_rows = [row for row in rows if row["group"] not in validation_groups]
    validation_rows = [row for row in rows if row["group"] in validation_groups]
    train_classes = {row["class"] for row in train_rows}
    validation_classes = {row["class"] for row in validation_rows}
    if set(classes) - train_classes or set(classes) - validation_classes:
        raise DatasetError("Could not place every species in both the training and validation sets")
    return train_rows, validation_rows


def train(args):
    random.seed(args.seed)
    torch.manual_seed(args.seed)
    if torch.cuda.is_available():
        torch.cuda.manual_seed_all(args.seed)
    torch.set_num_threads(max(1, args.threads))

    dataset_root = Path(args.dataset).expanduser().resolve()
    output_model = Path(args.output).expanduser().resolve()
    output_model.parent.mkdir(parents=True, exist_ok=True)

    rows, classes, metadata = read_export(dataset_root)
    train_rows, validation_rows = split_by_source(rows, classes, args.validation, args.seed)
    print(f"DATASET images={len(rows)} classes={len(classes)} train={len(train_rows)} validation={len(validation_rows)}", flush=True)
    print(f"CLASSES {json.dumps(classes, ensure_ascii=False)}", flush=True)

    training_transform = transforms.Compose([
        transforms.RandomResizedCrop(224, scale=(0.7, 1.0)),
        transforms.RandomHorizontalFlip(),
        transforms.ToTensor(),
        transforms.Normalize(mean=(0.485, 0.456, 0.406), std=(0.229, 0.224, 0.225)),
    ])
    validation_transform = transforms.Compose([
        transforms.Resize((224, 224)),
        transforms.ToTensor(),
        transforms.Normalize(mean=(0.485, 0.456, 0.406), std=(0.229, 0.224, 0.225)),
    ])
    class_to_index = {name: index for index, name in enumerate(classes)}
    train_data = SpeciesImageDataset(train_rows, class_to_index, training_transform)
    validation_data = SpeciesImageDataset(validation_rows, class_to_index, validation_transform)
    device = torch.device("cuda" if torch.cuda.is_available() and not args.cpu else "cpu")
    train_loader = DataLoader(train_data, batch_size=args.batch_size, shuffle=True, num_workers=0)
    validation_loader = DataLoader(validation_data, batch_size=args.batch_size, shuffle=False, num_workers=0)

    weights = models.ResNet18_Weights.DEFAULT if args.pretrained else None
    model = models.resnet18(weights=weights)
    model.fc = nn.Linear(model.fc.in_features, len(classes))
    model.to(device)
    optimizer = torch.optim.AdamW(model.parameters(), lr=args.learning_rate)
    criterion = nn.CrossEntropyLoss()
    best_accuracy = -1.0
    best_state = None

    for epoch in range(1, args.epochs + 1):
        model.train()
        total_loss = 0.0
        seen = 0
        for images, labels in train_loader:
            images = images.to(device)
            labels = labels.to(device)
            optimizer.zero_grad(set_to_none=True)
            logits = model(images)
            loss = criterion(logits, labels)
            loss.backward()
            optimizer.step()
            total_loss += loss.item() * images.size(0)
            seen += images.size(0)

        model.eval()
        correct = 0
        validation_seen = 0
        with torch.no_grad():
            for images, labels in validation_loader:
                logits = model(images.to(device))
                correct += (logits.argmax(dim=1).cpu() == labels).sum().item()
                validation_seen += labels.size(0)
        average_loss = total_loss / max(1, seen)
        accuracy = correct / max(1, validation_seen)
        print(f"EPOCH {epoch}/{args.epochs} loss={average_loss:.6f} validation_accuracy={accuracy:.6f}", flush=True)
        if accuracy > best_accuracy:
            best_accuracy = accuracy
            best_state = {key: value.detach().cpu().clone() for key, value in model.state_dict().items()}

    if best_state is None:
        raise DatasetError("Training did not produce a model checkpoint")
    model.load_state_dict(best_state)
    model.to("cpu").eval()

    temporary_model = output_model.with_suffix(output_model.suffix + ".tmp")
    sample = torch.zeros((1, 3, 224, 224), dtype=torch.float32)
    torch.onnx.export(
        model,
        sample,
        temporary_model,
        input_names=["images"],
        output_names=["logits"],
        dynamic_axes={"images": {0: "batch"}, "logits": {0: "batch"}},
        opset_version=17,
        dynamo=False,
    )
    temporary_model.replace(output_model)

    labels_path = output_model.with_suffix(".labels.json")
    labels_path.write_text(json.dumps({
        "format_version": 1,
        "architecture": "resnet18",
        "input_size": [224, 224],
        "normalization": {"mean": [0.485, 0.456, 0.406], "std": [0.229, 0.224, 0.225]},
        "class_names": classes,
        "species": metadata,
        "validation_accuracy": best_accuracy,
    }, ensure_ascii=False, indent=2), encoding="utf-8")
    print(f"COMPLETE model={output_model} labels={labels_path} validation_accuracy={best_accuracy:.6f}", flush=True)


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dataset", required=True, help="AIML_ImageExtractor dataset export folder")
    parser.add_argument("--output", required=True, help="Output ONNX model path")
    parser.add_argument("--epochs", type=int, default=10)
    parser.add_argument("--batch-size", type=int, default=16)
    parser.add_argument("--learning-rate", type=float, default=0.0001)
    parser.add_argument("--validation", type=float, default=0.2)
    parser.add_argument("--seed", type=int, default=42)
    parser.add_argument("--threads", type=int, default=4)
    parser.add_argument("--cpu", action="store_true", help="Force CPU training")
    parser.add_argument("--no-pretrained", dest="pretrained", action="store_false", help="Train without ImageNet weights")
    parser.set_defaults(pretrained=True)
    args = parser.parse_args()
    if args.epochs < 1 or args.batch_size < 1 or args.threads < 1:
        parser.error("epochs, batch-size, and threads must be positive")
    if not 0.05 <= args.validation <= 0.5:
        parser.error("validation must be between 0.05 and 0.5")
    if args.learning_rate <= 0:
        parser.error("learning-rate must be positive")
    return args


if __name__ == "__main__":
    try:
        train(parse_args())
    except DatasetError as error:
        print(f"ERROR: {error}", file=sys.stderr, flush=True)
        sys.exit(2)
    except Exception as error:
        print(f"ERROR: {type(error).__name__}: {error}", file=sys.stderr, flush=True)
        raise
