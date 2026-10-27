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

To run the worker directly:

```powershell
python train_species.py --dataset C:\data\bird-export --output C:\models\birds.onnx --epochs 10 --batch-size 16
```
