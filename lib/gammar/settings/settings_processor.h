//
// Created by bnunnally on 8/27/21.
//

#ifndef SHARP_SETTINGS_PROCESSOR_H
#define SHARP_SETTINGS_PROCESSOR_H

#include "settings.h"

void remove_ignored_files();
void validate_and_add_source_files(string &libPath);
void process_library_files(string versionNumber);

#endif //SHARP_SETTINGS_PROCESSOR_H
