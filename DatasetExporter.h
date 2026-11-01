// DatasetExporter.h : exports the stored images as a machine-learning dataset
//

#pragma once

#include "SpeciesDatabase.h"

// Writes <folder>\<Swedish>__<Latin>\img<id>.png for each image plus <folder>\manifest.csv
// (image_id,file,species_id,swedish,latin,english,source_file,frame,x,y,w,h,age_code).
// Returns the number of images exported, or -1 on failure (error describes why).
// Images labelled Unknown are never exported.
// minSources > 0 skips species whose images come from fewer than that many distinct source files (the trainer
// needs at least 2 for its source-aware validation split); skipped species names are appended to skipped if given.
int ExportDataset(CSpeciesDatabase& database, const CString& folder, CString& error,
	int minSources = 0, std::vector<CString>* skipped = nullptr);