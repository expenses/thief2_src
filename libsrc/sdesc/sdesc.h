#pragma once
#include <sdesbase.h>

const sFieldDesc* StructDescFindField(const sStructDesc* desc, const char* name);

BOOL StructFieldToString(const void* struc, const sStructDesc* desc, const sFieldDesc* field, char* dest);
BOOL StructStringToField(void* struc, const sStructDesc* desc, const sFieldDesc* field, char* src);

BOOL StructSetField(char* struc, const sStructDesc* desc, const sFieldDesc* field, void* val);
BOOL StructGetField(char* struc, const sStructDesc* desc, const sFieldDesc* field, void* val);

void StructDumpStruct(const void* struc, const sStructDesc* desc);

BOOL StructDescIsSimple(const sStructDesc* desc);
BOOL StructToSimpleString(const void* struc, const sStructDesc* desc, char* out);
BOOL StructFromSimpleString(const void* struc, const sStructDesc* desc, char* in);

BOOL StructToFullString(const void* struc, const sStructDesc* desc, char* out, int len);
BOOL StructFromFullString(const void* struc, const sStructDesc* desc, const char* in);

void sd_stuff_from_long(int* val, long in, ulong size);
long sd_cast_to_long(void* val, ulong size, BOOL is_signed);

void get_field(char* struc, void* newdata, const sFieldDesc* desc);
void set_field(char* struc, void* newdata, const sFieldDesc* desc);