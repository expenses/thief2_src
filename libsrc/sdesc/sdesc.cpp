#include <windows.h>
#include <algorithm>
#include <cstdio>
#include <cmath>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>

#include <sdesc.h>
#include <dbg.h>
#include <mprintf.h>
#include <mxang.h>

#ifndef NO_DB_MEM
// Must be last header
#include <memall.h>
#include <dbmem.h>
#endif

#define clamp(x, lower, upper)	max(lower, min(x, upper))

static const char* type_strings[] =
{
  "int", // kFieldTypeInt
  "BOOL", // kFieldTypeBool
  "short", // kFieldTypeShort
  "bitfield", // kFieldTypeBits
  "enum", // kFieldTypeEnum
  "char[]", // kFieldTypeString
  "char*", // kFieldTypeStringPtr
  "void*", // kFieldTypeVoidPtr
  "Point", // kFieldTypePoint
  "mxs_vector", // kFieldTypeVector
  "float", // kFieldTypeFloat
  "fix", // kFieldTypeFix
  "fixvec", // kFieldTypeFixVec
  "double", // kFieldTypeDouble
  "rgba", // kFieldTypeRGBA
  "doublevec", // kFieldTypeDoubleVec
  "ang", // kFieldTypeAng
  "angvec" // kFieldTypeAngVec
};

static const char* type_names[] =
{
  "Integer",
  "Boolean",
  "Short Integer",
  "Bitfield",
  "Enumeration",
  "String",
  "String Pointer",
  "Void Pointer",
  "2D Point",
  "3D Vector",
  "Floating-point Number",
  "Fixed-point Number",
  "Fixed-point Vector",
  "Double-precision Float",
  "24 bit RGB Color",
  "Double Vector",
  "16 bit Angle",
  "16 bit Angle Vector"
};


static void unparse_fixvec(const sFieldDesc* desc, const void* rawVal, char* out);
static void parse_double(const sFieldDesc* desc, void* val, const char* in);
static void unparse_double(const sFieldDesc* desc, const void* val, char* out);


const sFieldDesc* StructDescFindField(const sStructDesc* desc, const char* name)
{
	int which;
	for (which = 0; which < desc->nfields && strnicmp(name, desc->fields[which].name, 0x20u); ++which)
		;

	if (which < desc->nfields)
		return &desc->fields[which];

	Warning(("StructDescFindField: Field %s not found in struct %s\n", name, desc->name));
	return NULL;
}

long sd_cast_to_long(void* val, ulong size, BOOL is_signed)
{
	ulong sz = size;
	if (is_signed)
		sz = size + 4;

	switch (sz)
	{
	case 1:
		return *reinterpret_cast<const uint8*>(val);
	case 2:
		return *reinterpret_cast<const uint16*>(val);
	case 4:
		return *reinterpret_cast<const uint32*>(val);
	case 5:
		return *reinterpret_cast<const int8*>(val);
	case 6:
		return *reinterpret_cast<const int16*>(val);
	case 8:
		return *reinterpret_cast<const int32*>(val);
	default:
		Warning(("Invalid size for integer %d\n", size % 4));
		return 0;
	}
}

void sd_stuff_from_long(int* val, long in, ulong size)
{
	memcpy(val, &in, size);
}

static void parse_int(const sFieldDesc* desc, void* val, const char* in)
{
	int radix = 10;
	if ((desc->flags & kFieldFlagHex) != 0)
		radix = 16;
	long ival = strtol(in, 0, radix);
	sd_stuff_from_long((int*)val, ival, desc->size);
}

static void unparse_int(const sFieldDesc* desc, const void* val, char* out)
{
	long ival = sd_cast_to_long((void*)val, desc->size, (desc->flags & kFieldFlagUnsigned) == 0);
	int radix = 10;
	if ((desc->flags & kFieldFlagHex) != 0)
		radix = 16;
	ltoa(ival, out, radix);
}

static void parse_bool(const sFieldDesc* desc, void* val, const char* in)
{
	static const char* true_names[] = { "t", "y", "true", "yes", "on", NULL };
	static const char* false_names[] = { "f", "n", "false", "no", "off", NULL };
    const char** name;

	for (name = true_names; *name; ++name)
	{
		if (!strnicmp(in, *name, strlen(*name)))
		{
			sd_stuff_from_long((int*)val, 1, desc->size);
			return;
		}
	}

	for (name = false_names; *name; ++name)
	{
		if (!strnicmp(in, *name, strlen(*name)))
		{
			sd_stuff_from_long((int*)val, 0, desc->size);
			return;
		}
	}

	parse_int(desc, val, in);
}

static void unparse_bool(const sFieldDesc* desc, const void* val, char* out)
{
	sprintf(out, "%s", sd_cast_to_long((void*)val, desc->size, 0) ? "TRUE" : "FALSE");
}

static void parse_bits(const sFieldDesc* desc, void* val, const char* in)
{
	int ival = 0;
	if (desc->datasize > 0)
	{
		char** strings = (char**)desc->data;
		const char* s;
		for (s = in; isspace(*s); ++s)
			;
		while (*s)
		{
			const char* p = strchr(s, ',');
			if (!p) {
			    p = &s[strlen(s) + 1];
			}
			char tmp = *p;
			*const_cast<char*>(p) = '\0';

			for (int i = 0; i < desc->datasize; ++i)
			{
				if (!strcmp(s, strings[i]))
				{
					ival |= 1 << (desc->min + i);
					break;
				}
			}
			*const_cast<char*>(p) = tmp;
			if (!*p)
				break;
			for (s = p + 1; isspace(*s); ++s)
				;
		}
	}
	else
	{
		ival = strtol(in, 0, 2);
	}

	sd_stuff_from_long((int*)val, ival, desc->size);
}

static void unparse_bits(const sFieldDesc* desc, const void* val, char* out)
{
	if ((desc->flags & kFieldFlagHex) != 0)
	{
		unparse_int(desc, val, out);
	}
	else if (desc->datasize > 0)
	{
		int ival = sd_cast_to_long((void*)val, desc->size, 0);
		int comma = 0;
		char* s = out;
		char** strings = (char**)desc->data;
		*out = 0;
		for (int i = 0; i < desc->datasize; ++i)
		{
			if (((1 << (desc->min + i)) & ival) != 0)
			{
				if (comma)
				{
					static const char* commastr = ", ";
					strcpy(s, commastr);
					s += strlen(s);
				}
				strcpy(s, strings[i]);
				s += strlen(s);
				comma = 1;
			}
		}
		if (!comma)
			strcpy(out, "[None]");
	}
	else
	{
		int ival = sd_cast_to_long((void*)val, desc->size, 0);
		if (desc->max)
			ival &= (1 << desc->max) - 1;
		ltoa(ival, out, 2);
	}
}

static void parse_enum(const sFieldDesc* desc, void* val, const char* in)
{
	char** vec = (char**)desc->data;
	int found = 0;
	int i;
	for (i = 0; i < desc->datasize; ++i)
	{
		if (!stricmp(in, vec[i]))
		{
			found = 1;
			i += desc->min;
			break;
		}
	}

	if (!found)
	{
		parse_int(desc, val, in);
		i = sd_cast_to_long((void*)val, desc->size, 1);
		i = clamp(i, desc->min, desc->max);
	}

	sd_stuff_from_long((int*)val, i, desc->size);
}

static void unparse_enum(const sFieldDesc* desc, const void* val, char* out)
{
	long ival = sd_cast_to_long((void*)val, desc->size, (desc->flags & kFieldFlagUnsigned) == 0);
	int i = clamp(static_cast<int>(ival), desc->min, desc->max);

	if (i - desc->min > desc->datasize)
		unparse_int(desc, val, out);
	else
		strcpy(out, *((const char**)desc->data + i - desc->min));
}

// Inside StructToFullString/StructFromFullString the "{ a; b }" syntax characters in string fields are escaped
BOOL g_bSdescEscapeStrings;
static int gEscapeDepth;

static void EscapeString(char* out, const char* in, int len)
{
	if (!out)
		return;
	if (in)
	{
		char c = *in;
		--len;
		for (; c; c = *in)
		{
			if (len <= 0)
				break;
			switch (c)
			{
				case ';':
					if (len < 2) { len = 0; break; }
					*out++ = '$'; *out++ = ':'; ++in; len -= 2;
					break;
				case '{':
					if (len < 2) { len = 0; break; }
					*out++ = '$'; *out++ = '('; ++in; len -= 2;
					break;
				case '}':
					if (len < 2) { len = 0; break; }
					*out++ = '$'; *out++ = ')'; ++in; len -= 2;
					break;
				case '$':
					if (len < 2) { len = 0; break; }
					*out++ = '$'; *out++ = '$'; ++in; len -= 2;
					break;
				default:
					*out++ = c; ++in; --len;
					break;
			}
		}
	}
	*out = '\0';
}

static void UnescapeString(char* out, const char* in, int len)
{
	if (!out)
		return;
	if (in)
	{
		char c = *in;
		--len;
		for (; c; c = *in, --len)
		{
			if (len <= 0)
				break;
			if (c == '$')
			{
				++in;
				switch (*in)
				{
					case ':': *out = ';'; break;
					case '(': *out = '{'; break;
					case ')': *out = '}'; break;
					case '$': *out = '$'; break;
					default:
						*out++ = '$';
						--len;
						if (len <= 0 || !*in)
							goto next;
						*out = *in;
						break;
				}
			}
			else
				*out = c;
			++out;
			++in;
next:
			;
		}
	}
	*out = '\0';
}

static void parse_string(const sFieldDesc* desc, void* val, const char* in)
{
	if (gEscapeDepth)
	{
		UnescapeString(static_cast<char*>(val), in, desc->size);
		return;
	}
	strncpy(static_cast<char*>(val), in, desc->size);
	static_cast<char*>(val)[desc->size - 1] = '\0';
}

static void unparse_string(const sFieldDesc* desc, const void* val, char* out)
{
	if (gEscapeDepth)
	{
		EscapeString(out, static_cast<const char*>(val), desc->size);
		return;
	}
	strncpy(out, static_cast<const char*>(val), desc->size);
	if (desc->size > 0)
		out[desc->size - 1] = '\0';
}

static void parse_stringptr(const sFieldDesc* desc, void* val, const char* in)
{
	if (gEscapeDepth)
	{
		UnescapeString(*static_cast<char**>(val), in, INT_MAX);
		return;
	}
	strcpy(*static_cast<char**>(val), in);
}

static void unparse_stringptr(const sFieldDesc* desc, const void* val, char* out)
{
	if (gEscapeDepth)
	{
		EscapeString(out, *static_cast<char* const*>(val), INT_MAX);
		return;
	}
	strcpy(out, *static_cast<char* const*>(val));
}

static void parse_voidptr(const sFieldDesc* desc, void* val, const char* in)
{
	sscanf(in, "%p", val);
}

static void unparse_voidptr(const sFieldDesc* desc, const void* val, char* out)
{
	sprintf(out, "%p", val);
}

static void parse_point(const sFieldDesc* desc, void* val, const char* in)
{
	sFieldDesc idesc = *desc;
	idesc.size >>= 1;

	parse_int(&idesc, val, in);

	const char* comma = strchr(in, ',');
	if (comma)
		parse_int(&idesc, (int*)((char*)val + idesc.size), comma + 1);
}

static void unparse_point(const sFieldDesc* desc, const void* val, char* out)
{
	sFieldDesc idesc = *desc;
	idesc.size >>= 1;

	unparse_int(&idesc, val, out);
	strcat(out, ", ");
	unparse_int(&idesc, (int*)((char*)val + idesc.size), &out[strlen(out)]);
}

static void parse_vector(const sFieldDesc* desc, void* rawVal, const char* in)
{
	float* val = reinterpret_cast<float*>(rawVal);
	sscanf(in, "%f, %f, %f", &val[0], &val[1], &val[2]);
}

static void unparse_vector(const sFieldDesc* desc, const void* rawVal, char* out)
{
	const float* val = reinterpret_cast<const float*>(rawVal);
	sprintf(out, "%0.2f, %0.2f, %0.2f", val[0], val[1], val[2]);
}

static void parse_rgba(const sFieldDesc* desc, void* rawVal, const char* in)
{
	uchar* val = reinterpret_cast<uchar*>(rawVal);
	sscanf(in, "%d, %d, %d", &val[1], &val[2], &val[3]);
}

static void unparse_rgba(const sFieldDesc* desc, const void* rawVal, char* out)
{
	const uchar* val = reinterpret_cast<const uchar*>(rawVal);
	sprintf(out, "%d, %d, %d", val[1], val[2], val[3]);
}

static void parse_double_vec(const sFieldDesc* desc, void* rawVal, const char* in)
{
	double* val = reinterpret_cast<double*>(rawVal);
	sscanf(in, "%lf, %lf, %lf", &val[0], &val[1], &val[2]);
}

static void unparse_double_vec(const sFieldDesc* desc, const void* rawVal, char* out)
{
	const double* val = reinterpret_cast<const double*>(rawVal);
	sprintf(out, "%0.4lf, %0.4lf, %0.4lf", val[0], val[1], val[2]);
}

static void parse_ang(const sFieldDesc* desc, void* val, const char* in)
{
	*reinterpret_cast<ushort*>(val) = (ushort)floor(atof(in) * 32768.0 / 180.0 + 0.5);
}

static void unparse_ang(const sFieldDesc* desc, const void* val, char* out)
{
	sprintf(out, "%0.2f", *reinterpret_cast<const ushort*>(val) * (180.0 / 32768.0));
}

static void parse_ang_vec(const sFieldDesc* desc, void* rawVal, const char* in)
{
	mxs_angvec* val = reinterpret_cast<mxs_angvec*>(rawVal);

	double z = 0, y = 0, x = 0;
	sscanf(in, "%lf, %lf, %lf", &x, &y, &z);
	val->tz = floor(x * 32768.0 / 180.0 + 0.5);
	val->ty = floor(y * 32768.0 / 180.0 + 0.5);
	val->tx = floor(z * 32768.0 / 180.0 + 0.5);
}

static void unparse_ang_vec(const sFieldDesc* desc, const void* rawVal, char* out)
{
	const mxs_angvec* val = reinterpret_cast<const mxs_angvec*>(rawVal);
	sprintf(out, "%0.2f, %0.2f, %0.2f",
		val->tz * 180.0 / 32768.0,
		val->ty * 180.0 / 32768.0,
		val->tx * 180.0 / 32768.0);
}

static void parse_float(const sFieldDesc* desc, void* val, const char* in)
{
	sscanf(in, "%f", reinterpret_cast<float*>(val));
}

static void unparse_float(const sFieldDesc* desc, const void* val, char* out)
{
	sprintf(out, "%0.2f", *reinterpret_cast<const float*>(val));
}

static void parse_fix(const sFieldDesc* desc, void* val, const char* in)
{
	float num = 0;
	sscanf(in, "%f", &num);
	*reinterpret_cast<long*>(val) = (long)(num * 65536.0);
}

static void unparse_fix(const sFieldDesc* desc, const void* val, char* out)
{
	sprintf(out, "%0.2f", *reinterpret_cast<const long*>(val) * (1.0f / 65536.0f));
}

static void parse_fixvec(const sFieldDesc* desc, void* rawVal, const char* in)
{
	float vec[3] = { 0 };
	long* val = reinterpret_cast<long*>(rawVal);

	sscanf(in, "%f %f %f", &vec[0], &vec[1], &vec[2]);
	val[0] = (long)(vec[0] * 65536.0);
	val[1] = (long)(vec[1] * 65536.0);
	val[2] = (long)(vec[2] * 65536.0);
}

static void(*parse_funcs[])(const sFieldDesc*, void*, const char*) =
{
  &parse_int,			// kFieldTypeInt
  &parse_bool,			// kFieldTypeBool
  &parse_int,			// kFieldTypeShort
  &parse_bits,			// kFieldTypeBits
  &parse_enum,			// kFieldTypeEnum
  &parse_string,		// kFieldTypeString
  &parse_stringptr,		// kFieldTypeStringPtr
  &parse_voidptr,		// kFieldTypeVoidPtr
  &parse_point,			// kFieldTypePoint
  &parse_vector,		// kFieldTypeVector
  &parse_float,			// kFieldTypeFloat
  &parse_fix,			// kFieldTypeFix
  &parse_fixvec,		// kFieldTypeFixVec
  &parse_double,		// kFieldTypeDouble
  &parse_rgba,			// kFieldTypeRGBA
  &parse_double_vec,	// kFieldTypeDoubleVec
  &parse_ang,			// kFieldTypeAng
  &parse_ang_vec		// kFieldTypeAngVec
};

static void(*unparse_funcs[])(const sFieldDesc*, const void*, char*) =
{
  &unparse_int,
  &unparse_bool,
  &unparse_int,
  &unparse_bits,
  &unparse_enum,
  &unparse_string,
  &unparse_stringptr,
  &unparse_voidptr,
  &unparse_point,
  &unparse_vector,
  &unparse_float,
  &unparse_fix,
  &unparse_fixvec,
  &unparse_double,
  &unparse_rgba,
  &unparse_double_vec,
  &unparse_ang,
  &unparse_ang_vec
};

static void unparse_fixvec(const sFieldDesc* desc, const void* rawVal, char* out)
{
	const long* val = reinterpret_cast<const long*>(rawVal);
	sprintf(out, "%0.2f, %0.2f, %0.2f", val[0] * (1.0f / 65536.0f), val[1] * (1.0f / 65536.0f), val[2] * (1.0f / 65536.0f));
}

static void parse_double(const sFieldDesc* desc, void* val, const char* in)
{
	sscanf(in, "%lf", reinterpret_cast<double*>(val));
}

static void unparse_double(const sFieldDesc* desc, const void* val, char* out)
{
	sprintf(out, "%0.8lf", *reinterpret_cast<const double*>(val));
}

int StructSetField(char* struc, const sStructDesc* desc, const sFieldDesc* field, void* val)
{
	if (!desc || !struc || !val || !field)
	{
		Warning(("StructSetField: Failed because of NULL pointer parameter\n"));
		return FALSE;
	}

	set_field(struc, val, field);
	return TRUE;
}

int StructGetField(char* struc, const sStructDesc* desc, const sFieldDesc* field, void* val)
{
	if (!desc || !struc || !val || !field)
	{
		Warning(("StructGetField: Failed because of NULL pointer parameter\n"));
		return FALSE;
	}

	get_field(struc, val, field);
	return TRUE;
}

BOOL StructFieldToString(const void* struc, const sStructDesc* desc, const sFieldDesc* field, char* dest)
{
	if (!field || !desc || !struc || !dest)
		return FALSE;

	unparse_funcs[field->type](field, (const char*)struc + field->offset, dest);

	return TRUE;
}

BOOL StructStringToField(void* struc, const sStructDesc* desc, const sFieldDesc* field, char* src)
{
	if (!field || !desc || !struc || !src)
		return FALSE;

	parse_funcs[field->type](field, (char*)struc + field->offset, src);

	return TRUE;
}

void StructDumpStruct(const void* struc, const sStructDesc* desc)
{
	if (!desc || !struc)
		return;

	mprintf("Struct %s, size %d, flags %x, nfields %d\n", desc->name, desc->size, desc->flags, desc->nfields);

	for (int i = 0; i < desc->nfields; ++i)
	{
		char str[256];

		StructFieldToString(struc, desc, &desc->fields[i], str);
		mprintf(
			"  ->%s (type %s, size %d) = %s\n",
			desc->fields[i].name,
			type_strings[desc->fields[i].type],
			desc->fields[i].size,
			str);
	}
}

BOOL StructDescIsSimple(const sStructDesc* desc)
{
	if (!desc)
		return TRUE;
	return desc->nfields == 1;
}

BOOL StructToSimpleString(const void* struc, const sStructDesc* desc, char* out)
{
	switch (desc->nfields)
	{
	case 0:
		strcpy(out, "-");
		return TRUE;
	case 1:
		unparse_funcs[desc->fields->type](desc->fields, (char*)struc + desc->fields->offset, out);
		return TRUE;
	default:
		strcpy(out, "...");
		return FALSE;
	}
}

BOOL StructFromSimpleString(const void* struc, const sStructDesc* desc, char* in)
{
	switch (desc->nfields)
	{
	case 0:
		return TRUE;
	case 1:
		parse_funcs[desc->fields->type](desc->fields, (char*)struc + desc->fields->offset, in);
		return TRUE;
	default:
		return FALSE;
	}
}


void get_field(char* struc, void* newdata, const sFieldDesc* desc)
{
	memcpy(newdata, &((char*)struc)[desc->offset], desc->size);
}

void set_field(char* struc, void* newdata, const sFieldDesc* desc)
{
	memcpy(&((char*)struc)[desc->offset], newdata, desc->size);
}

BOOL StructToFullString(const void* struc, const sStructDesc* desc, char* out, int len)
{
	int too_long = 0;
	char* p;

	if (!desc->nfields)
	{
		strncpy(out, "-", len);
	}
	else
	{
		int bufsiz = 256;
		char* buf = static_cast<char*>(Malloc(0x100));
		p = out;
		if (desc->nfields > 1)
		{
			strcpy(out, "{ ");
			p = &out[strlen(out)];
		}
		if (g_bSdescEscapeStrings)
			++gEscapeDepth;
		for (int i = 0; i < desc->nfields; ++i)
		{
			if (desc->fields[i].size > bufsiz)
			{
				while (bufsiz < desc->fields[i].size)
					bufsiz *= 2;

				buf = static_cast<char*>(Realloc(buf, bufsiz));
			}
			if (i)
			{
				static char* delim = "; ";
				if (&p[strlen(delim) + 1] > &out[len])
				{
					too_long = 1;
					break;
				}
				strcpy(p, delim);
				p += strlen(delim);
			}
			unparse_funcs[desc->fields[i].type](&desc->fields[i], &((char*)struc)[desc->fields[i].offset], buf);
			strncpy(p, buf, &out[len] - p);
			if (&p[strlen(buf) + 1] > &out[len])
			{
				too_long = 1;
				break;
			}
			p += strlen(buf);
		}
		if (g_bSdescEscapeStrings)
			--gEscapeDepth;

		Free(buf);

		if (desc->nfields > 1 && !too_long)
		{
			if (p + 2 <= &out[len])
				strcat(out, "}");
			else
				too_long = 1;
		}

		if (too_long)
		{
			static char* elipsis = "...";
			strncpy(&out[len - 1 - strlen(elipsis)], elipsis, strlen(elipsis) + 1);
		}
	}
	return TRUE;
}

BOOL StructFromFullString(const void* struc, const sStructDesc* desc, const char* in)
{
	if (desc->nfields == 1)
	{
		parse_funcs[desc->fields->type](desc->fields, &((char*)struc)[desc->fields->offset], in);
	}
	else if (desc->nfields > 1)
	{
		int i = 0;
		const char* p = strchr(in, '{');
		++gEscapeDepth;
		for (; p && i < desc->nfields; ++i)
		{
			while (isspace(*++p))
				;
			const char* pa = p;
			const char* delim = &pa[strcspn(pa, ";}")];
			char tmp = *delim;
			*const_cast<char*>(delim) = '\0';
			parse_funcs[desc->fields[i].type](&desc->fields[i], &((char*)struc)[desc->fields[i].offset], pa);
			*const_cast<char*>(delim) = tmp;
			p = delim;
		}
		--gEscapeDepth;
	}

	return TRUE;
}
