#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef ALIAS_ENABLE_MIMALLOC
#include <mimalloc.h>
#endif

#include "object.h"

#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

#define MAP_BUFFER_SIZE (1024u * 1024u)

typedef struct
{
    char From[MAP_BUFFER_SIZE];
    char To[MAP_BUFFER_SIZE];
} ALIAS_MAP_BUFFER;

typedef struct
{
    PCSTR From, To;
} ALIAS_ENTRY;

typedef struct
{
    FILE* Obj;
    DWORD StringTableLength, SymbolCount;
    PCSTR* StringArray;
    ALIAS_ENTRY** Aliases;
    ALIAS_MAP_BUFFER* MapBuffer;
} ALIAS_CONTEXT;

void InitAliasContext(ALIAS_CONTEXT* const Context, PCSTR const OutputFile)
{
    assert(Context);
    assert(OutputFile);

    Context->Obj = fopen(OutputFile, "wb");
    Context->StringTableLength = sizeof(DWORD);
    Context->StringArray = NULL;
    Context->SymbolCount = 0;
    Context->Aliases = NULL;

    if (!Context->Obj)
    {
        printf("error: can't open output file \"%s\" for writing\n", OutputFile);
        exit(1);
    }
}

void DisposeAliasContext(ALIAS_CONTEXT* const Context)
{
    assert(Context);

    assert(!Context->MapBuffer);

    stbds_arrfree(Context->StringArray);
    stbds_arrfree(Context->Aliases);
    fclose(Context->Obj);
}

PCSTR CopyString(PCSTR const Source)
{
    size_t Length;
    PSTR Copy;

    assert(Source);

    Length = strlen(Source) + 1;

    assert(Length >= 1);

    Copy = malloc(Length);

    memmove(Copy, Source, Length);

    return (PCSTR)Copy;
}

void StoreString(ALIAS_CONTEXT* const Context, PCSTR const String, IMAGE_SYMBOL* const Symbol)
{
    size_t Length;

    assert(Context);
    assert(String);
    assert(Symbol);

    Length = strlen(String);

    assert(Length >= 1);

    RtlSecureZeroMemory((void*)&Symbol->N.Name, sizeof Symbol->N.Name);

    if (Length <= IMAGE_SIZEOF_SHORT_NAME)
    {
        strncpy_s((char*)Symbol->N.ShortName, IMAGE_SIZEOF_SHORT_NAME, String, IMAGE_SIZEOF_SHORT_NAME);
    }
    else
    {
        stbds_arrpush(Context->StringArray, CopyString(String));
        Symbol->N.Name.Long = Context->StringTableLength;
        Context->StringTableLength += Length + 1;
    }
}

void WriteHeader(const ALIAS_CONTEXT* const Context)
{
    IMAGE_FILE_HEADER File = {0};
    IMAGE_SECTION_HEADER Section = {0};

    assert(Context);

    File.Machine = IMAGE_FILE_MACHINE_UNKNOWN;
    File.NumberOfSections = 1;
    File.TimeDateStamp = (DWORD)time(NULL);
    File.PointerToSymbolTable = sizeof File + sizeof Section;
    File.NumberOfSymbols = stbds_arrlenu(Context->Aliases) * 3u;
    File.SizeOfOptionalHeader = 0;
    File.Characteristics = 0;

    fwrite(&File, sizeof File, 1, Context->Obj);

    strcpy_s(Section.Name, IMAGE_SIZEOF_SHORT_NAME, ".text");
    Section.Characteristics = IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_EXECUTE;

    fwrite(&Section, sizeof Section, 1, Context->Obj);
}

void WriteStringTable(const ALIAS_CONTEXT* const Context)
{
    DWORD Pos;

    assert(Context);

    Pos = ftell(Context->Obj);

    fwrite(&Context->StringTableLength, sizeof Context->StringTableLength, 1, Context->Obj);

    for (size_t i = 0; i < stbds_arrlenu(Context->StringArray); ++i)
    {
        fputs(Context->StringArray[i], Context->Obj);
        fputc(0, Context->Obj);
        free((void*)Context->StringArray[i]);
        Context->StringArray[i] = NULL;
    }

    assert(ftell(Context->Obj) - Pos == Context->StringTableLength);
}

void WriteAlias(ALIAS_CONTEXT* const Context, PCSTR szFrom, PCSTR szTo)
{
    IMAGE_SYMBOL Symbol = { 0 };
    IMAGE_AUX_SYMBOL Aux = { 0 };

    assert(Context);
    assert(szFrom);
    assert(szTo);
    assert(strlen(szFrom) >= 1);
    assert(strlen(szTo) >= 1);

    const DWORD SourceSymbolIndex = Context->SymbolCount++;

    StoreString(Context, szTo, &Symbol);
    Symbol.Value = 0;
    Symbol.SectionNumber = IMAGE_SYM_UNDEFINED;
    Symbol.Type = IMAGE_SYM_TYPE_NULL;
    Symbol.StorageClass = IMAGE_SYM_CLASS_EXTERNAL;
    Symbol.NumberOfAuxSymbols = 0;
    fwrite(&Symbol, sizeof Symbol, 1, Context->Obj);

    StoreString(Context, szFrom, &Symbol);
    Symbol.Value = 0;
    Symbol.SectionNumber = IMAGE_SYM_UNDEFINED;
    Symbol.Type = IMAGE_SYM_TYPE_NULL;
    Symbol.StorageClass = IMAGE_SYM_CLASS_WEAK_EXTERNAL;
    Symbol.NumberOfAuxSymbols = 1;
    fwrite(&Symbol, sizeof Symbol, 1, Context->Obj);

    Context->SymbolCount++;

    memset(&Aux, 0, sizeof Aux);
    Aux.Misc.TotalSize = IMAGE_WEAK_EXTERN_SEARCH_ALIAS;
    Aux.TagIndex = SourceSymbolIndex;
    fwrite(&Aux, sizeof Aux, 1, Context->Obj);

    Context->SymbolCount++;
}

void TrimNewLine(PSTR const String)
{
    assert(String);

    String[strcspn(String, "\r\n")] = '\0';
}

BOOLEAN ReadNewLine(FILE* const File, PSTR const String)
{
    assert(File);
    assert(String);

    if (fgets(String, MAP_BUFFER_SIZE, File) == NULL)
        return FALSE;

    TrimNewLine(String);
    return TRUE;
}

BOOLEAN ReadAliasesFromFile(ALIAS_CONTEXT* const Context, FILE* Map)
{
    ALIAS_MAP_BUFFER* MapBuffer;

    assert(Context);
    assert(Map);

    MapBuffer = Context->MapBuffer;

    while (ReadNewLine(Map, MapBuffer->From) && ReadNewLine(Map, MapBuffer->To))
    {
        ALIAS_ENTRY* const Entry = calloc(1, sizeof(ALIAS_ENTRY));

        if (!Entry)
        {
            printf("error: can't allocate ALIAS_ENTRY\n");
            return FALSE;
        }

        Entry->From = CopyString(MapBuffer->From);
        Entry->To = CopyString(MapBuffer->To);

        printf("\"%s\" -> \"%s\"\n", Entry->From, Entry->To);

        stbds_arrpush(Context->Aliases, Entry);
    }

    return TRUE;
}

void ReadAliases(ALIAS_CONTEXT* const Context, PCSTR const MapPath)
{
    BOOLEAN Success;
    FILE* Map;

    assert(Context);
    assert(MapPath);

    if ((Map = fopen(MapPath, "r")) == NULL)
    {
        printf("error: can't open mappings file \"%s\"\n", MapPath);
        exit(1);
    }

    if (!Context->MapBuffer)
    {
        Context->MapBuffer = calloc(1, sizeof(ALIAS_MAP_BUFFER));
    }

    Success = ReadAliasesFromFile(Context, Map);

    if (ferror(Map))
    {
        printf("error: can't read mappings file \"%s\"\n", MapPath);
        Success = FALSE;
    }

    fclose(Map);

    if (!Success)
        exit(1);
}

int main(int argc, char** argv)
{
    PSTR ObjPath;
    ALIAS_CONTEXT Context = { 0 };

#ifdef ALIAS_ENABLE_MIMALLOC
    mi_option_enable(mi_option_show_errors);
    mi_option_enable(mi_option_show_stats);
    mi_option_enable(mi_option_verbose);
#endif

    if (argc < 3)
    {
        printf("usage: ALIASOBJ <output-file> <map-files>...\n");
        exit(1);
    }

    ObjPath = argv[1];

    InitAliasContext(&Context, ObjPath);

    for (int i = 2; i < argc; ++i)
        ReadAliases(&Context, argv[i]);

    if (Context.MapBuffer)
    {
        free(Context.MapBuffer);
        Context.MapBuffer = NULL;
    }

    WriteHeader(&Context);

    for (size_t i = 0; i < stbds_arrlenu(Context.Aliases); ++i)
    {
        ALIAS_ENTRY* const Alias = Context.Aliases[i];

        WriteAlias(&Context, Alias->From, Alias->To);

        free((void*)Alias->From);
        free((void*)Alias->To);

        Alias->From = NULL;
        Alias->To = NULL;

        free(Alias);

        Context.Aliases[i] = NULL;
    }

    WriteStringTable(&Context);

    DisposeAliasContext(&Context);

    return 0;
}
