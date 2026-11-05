// BatchImageList.h : image list files for ProcessImagesFromFile and their progress files
//

#pragma once

#include <string>
#include <vector>

// Reads a text file with one image path per line (UTF-8 with or without BOM, otherwise ANSI).
// Blank lines and lines starting with '#' are ignored; surrounding spaces and quotes are trimmed;
// relative paths are resolved against the folder of the list file. Returns false if the file cannot be read.
bool LoadImageList(const std::wstring& listFile, std::vector<std::wstring>& images);

// The progress file sits beside the list file: <listFile>.progress
std::wstring GetProgressFilePath(const std::wstring& listFile);

// Number of images already processed (0 if there is no readable progress file).
size_t LoadBatchProgress(const std::wstring& listFile);
bool SaveBatchProgress(const std::wstring& listFile, size_t processedCount);
void ClearBatchProgress(const std::wstring& listFile);
