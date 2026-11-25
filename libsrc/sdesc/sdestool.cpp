#include <algorithm>
#include <cmath>

#include <sdestool.h>
#include <sdesc.h>
#include <aggmemb.h>
#include <appagg.h>

#include <hashpp.h>
#include <hshpptem.h>

#include <cfgdbg.h>
#include <initguid.h>
#include <sdesguid.h>

class cSdescTools : public IStructDescTools
{
	DECLARE_AGGREGATION(cSdescTools);

public:
	cSdescTools(IUnknown* pOuter);

	//
	// Lookup a field by name
	//
	STDMETHOD_(const sFieldDesc*, GetFieldNamed)(const sStructDesc* desc, const char* field)
	{
		return StructDescFindField(desc, field);
	}

	//
	// Parse and unparse a field
	//
	STDMETHOD(ParseField)(const sFieldDesc* fdesc, const char* string, void* struc)
	{
		return StructStringToField(struc, reinterpret_cast<const sStructDesc*>(1), fdesc, (char*)string) ? S_OK : S_FALSE;
	}
	STDMETHOD(UnparseField)(const sFieldDesc* fdesc, const void* struc, char* string, int buflen)
	{
		int size = 1024;
		if (fdesc->size > (ulong)size)
			size = fdesc->size;
		if (size < buflen)
			size = buflen;

		char* buf = new char[size]();
		HRESULT retval = S_FALSE;
		if (StructFieldToString(struc, reinterpret_cast<const sStructDesc*>(1), fdesc, buf))
		{
			strncpy(string, buf, buflen);
			string[buflen - 1] = '\0';
			retval = S_OK;
		}

		delete[] buf;

		return retval;
	}

	//
	// Set and Get an integral field
	//
	STDMETHOD(SetIntegral)(const sFieldDesc* fdesc, long value, void* struc)
	{
		sd_stuff_from_long((int*)((char*)struc + fdesc->offset), value, fdesc->size);
		return S_OK;
	}
	STDMETHOD(GetIntegral)(const sFieldDesc* fdesc, const void* struc, long* value)
	{
		const char* p = (const char*)struc + fdesc->offset;
		*value = sd_cast_to_long((void*)p, fdesc->size, !(fdesc->flags & kFieldFlagUnsigned));
		return S_OK;
	}

	//
	// Parse & Unparse a "simple" structure (e.g. a one-field structure)
	//
	STDMETHOD_(BOOL, IsSimple)(const sStructDesc* desc)
	{
		return StructDescIsSimple(desc);
	}
	STDMETHOD_(BOOL, ParseSimple)(const sStructDesc* desc, const char* string, void* struc)
	{
		return StructFromSimpleString(struc, desc, (char*)string);
	}
	STDMETHOD_(BOOL, UnparseSimple)(const sStructDesc* desc, const void* struc, char* out, int len)
	{
		char* buf = new char[max(1024, len)];
		BOOL retval = StructToSimpleString(struc, desc, buf);

		strncpy(out, buf, len);
		out[len - 1] = '\0';

		delete[] buf;

		return retval;
	}

	//
	// Dump a struct to the mono
	//
	STDMETHOD(Dump)(const sStructDesc* sdesc, const void* struc)
	{
		StructDumpStruct(struc, sdesc);
		return S_OK;
	}


	//
	// Parse & unparse the "full" representation of a structure
	//

	STDMETHOD_(BOOL, ParseFull)(const sStructDesc* sdesc, const char* string, void* struc)
	{
		if (!sdesc || !string || !struc)
			return FALSE;
		return StructFromFullString(struc, sdesc, string);
	}
	STDMETHOD_(BOOL, UnparseFull)(const sStructDesc* sdesc, const void* struc, char* out, int buflen)
	{
		return StructToFullString(struc, sdesc, out, buflen);
	}



	//
	// Lookup an sdesc in the registry.  NULL if none exists
	//
	STDMETHOD_(const sStructDesc*, Lookup)(const char* name)
	{
		return mRegistry.Search(name);
	}
	STDMETHOD(Register)(const sStructDesc* desc)
	{
		mRegistry.Set(desc->name, desc);

		return S_OK;
	}
	STDMETHOD(ClearRegistry)()
	{
		mRegistry.Clear();

		return S_OK;
	}

protected:
	//
	// Aggregate protocol
	//

	HRESULT Init()
	{
		return S_OK;
	}
	HRESULT End()
	{
		ClearRegistry();
		return S_OK;
	}

private:
	cHashTable<const char*, const sStructDesc*, cHashFunctions> mRegistry;
};

IMPLEMENT_AGGREGATION_SELF_DELETE(cSdescTools);

static sRelativeConstraint Constraints[] =
{
   { kNullConstraint }
};

cSdescTools::cSdescTools(IUnknown* pOuter)
{
	INIT_AGGREGATION_1(pOuter, IID_IStructDescTools, this, kPriorityNormal, Constraints);
}

void SdescToolsCreate(void)
{
	AutoAppIPtr(Unknown);
	new cSdescTools(pUnknown);
}
