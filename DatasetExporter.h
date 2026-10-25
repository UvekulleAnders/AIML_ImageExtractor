// DatasetExporter.h : exports the stored images as a machine-learning dataset
//

#pragma once

#include "SpeciesDatabase.h"

// Writes <folder>\<Swedish>__<Latin>\img<id>.png for each image plus <folder>\manifest.csv
// (image_id,file,species_id,swedish,latin,english,source_file,frame,x,y,w,h).
// Returns the number of images exported, or -1 on failure (error describes why).
int ExportDataset(CSpeciesDatabase& database, const CString& folder, bool includeUnknown, CString& error);