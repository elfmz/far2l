#pragma once

void MaskGroupsSettings();
bool GetMaskGroup(const FARString &MaskName, FARString &MaskValue);
unsigned GetMaskGroupExpandRecursiveAll(FARString &fsMasks);
void CheckMaskGroups( void );
