# Species Model Trainer

A dialog-based MFC front end for training a bird-species image classifier from an AIML_ImageExtractor dataset export. Training runs in a separate Python/PyTorch process; the MFC application remains responsive and displays its output.

## Requirements

- Visual Studio with the Desktop development with C++ workload and MFC component.
- Python 3.10 or later on `PATH`.
- Python dependencies from `requirements.txt` installed into that interpreter.
- Network access the first time training starts with ImageNet pretrained weights, unless `--no-pretrained` is selected in a direct worker invocation.

Install dependencies with:

```powershell
python -m pip install -r requirements.txt
```

## Usage

1. In AIML_ImageExtractor, export the labeled image database using **File > Export dataset...**.
2. Start SpeciesModelTrainer and choose the export folder containing `manifest.csv`.
3. Choose an output path ending in `.onnx`, set epochs and batch size, then start training.
4. The trainer writes the ONNX model and a sibling `.labels.json` file. Keep both files together for the later inference integration.

The worker splits by source video rather than by individual frame. Every species therefore needs at least two distinct source videos for validation; this avoids putting near-duplicate frames from one video into both training and validation. The current model is ResNet18 fine-tuned with ImageNet weights, using 224x224 RGB input and ImageNet mean/std normalization. Training and validation accuracy are reported in the log; they are diagnostics, not a guarantee of field accuracy.

## Troubleshooting

Failures are reported in the **Training log** box of the trainer dialog (the large read-only text area below the Epochs/Batch size fields); the line above it shows a short status. The log shows everything the Python worker prints to its standard output and error, including Python tracebacks. There is no separate log file.

- `No module named PIL` (or `torch`, `torchvision`, `onnx`): a dependency is missing from the Python on `PATH`. Run `python -m pip install -r requirements.txt`. The module `PIL` is installed by the package `Pillow`. If `pip` is not recognised, use `python -m pip`.
- If the log shows nothing, run the worker directly (see below) in a terminal to see its output.

To run the worker directly:

```powershell
python train_species.py --dataset C:\data\bird-export --output C:\models\birds.onnx --epochs 10 --batch-size 16
```
