/*
 * aclui.h
 *
 * Access Control List Editor definitions
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef __ACLUI_H
#define __ACLUI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <unknwn.h>
#include <accctrl.h>
#include <commctrl.h>

typedef struct _SI_OBJECT_INFO
{
    DWORD     dwFlags;
    HINSTANCE hInstance;
    LPWSTR    pszServerName;
    LPWSTR    pszObjectName;
    LPWSTR    pszPageTitle;
    GUID      guidObjectType;
} SI_OBJECT_INFO, *PSI_OBJECT_INFO;

#define SI_EDIT_PERMS               0x00000000
#define SI_EDIT_OWNER               0x00000001
#define SI_EDIT_AUDITS              0x00000002
#define SI_CONTAINER                0x00000004
#define SI_READONLY                 0x00000008
#define SI_ADVANCED                 0x00000010
#define SI_RESET                    0x00000020
#define SI_OWNER_READONLY           0x00000040
#define SI_EDIT_PROPERTIES          0x00000080
#define SI_OWNER_RECURSE            0x00000100
#define SI_NO_ACL_PROTECT           0x00000200
#define SI_NO_TREE_APPLY            0x00000400
#define SI_PAGE_TITLE               0x00000800
#define SI_SERVER_IS_DC             0x00001000
#define SI_RESET_DACL_TREE          0x00004000
#define SI_RESET_SACL_TREE          0x00008000
#define SI_OBJECT_GUID              0x00010000
#define SI_EDIT_EFFECTIVE           0x00020000
#define SI_RESET_DACL               0x00040000
#define SI_RESET_SACL               0x00080000
#define SI_RESET_OWNER              0x00100000
#define SI_NO_ADDITIONAL_PERMISSION 0x00200000
#define SI_MAY_WRITE                0x10000000
#define SI_EDIT_ALL                 (SI_EDIT_OWNER |SI_EDIT_PERMS | SI_EDIT_AUDITS)

typedef struct _SI_ACCESS
{
    const GUID  *pguid;
    ACCESS_MASK mask;
    LPCWSTR     pszName;
    DWORD       dwFlags;
} SI_ACCESS, *PSI_ACCESS;

#define SI_ACCESS_SPECIFIC          0x00010000
#define SI_ACCESS_GENERAL           0x00020000
#define SI_ACCESS_CONTAINER         0x00040000
#define SI_ACCESS_PROPERTY          0x00080000

typedef struct _SI_INHERIT_TYPE
{
    const GUID *pguid;
    ULONG      dwFlags;
    LPCWSTR    pszName;
} SI_INHERIT_TYPE, *PSI_INHERIT_TYPE;

typedef enum _SI_PAGE_TYPE
{
    SI_PAGE_PERM = 0,
    SI_PAGE_ADVPERM,
    SI_PAGE_AUDIT,
    SI_PAGE_OWNER
} SI_PAGE_TYPE;

DEFINE_GUID(IID_ISecurityInformation, 0x965fc360, 0x16ff, 0x11d0, 0x0091, 0xcb,0x00,0xaa,0x00,0xbb,0xb7,0x23);
DEFINE_GUID(IID_ISecurityInformation2, 0xc3ccfdb4, 0x6f88, 0x11d2, 0x00a3, 0xce,0x00,0xc0,0x4f,0xb1,0x78,0x2a);
DEFINE_GUID(IID_IEffectivePermission, 0x3853dc76, 0x9f35, 0x407c, 0x0088, 0xa1,0xd1,0x93,0x44,0x36,0x5f,0xbc);
DEFINE_GUID(IID_ISecurityObjectTypeInfo, 0xfc3066eb, 0x79ef, 0x444b, 0x0091, 0x11,0xd1,0x8a,0x75,0xeb,0xf2,0xfa);

typedef interface ISecurityInformation *LPSECURITYINFO;
typedef interface ISecurityInformation2 *LPSECURITYINFO2;
typedef interface IEffectivePermission *LPEFFECTIVEPERMISSION;
typedef interface ISecurityObjectTypeInfo *LPSecurityObjectTypeInfo;

#undef INTERFACE
EXTERN_C const IID IID_ISecurityInformation;
#define INTERFACE ISecurityInformation
DECLARE_INTERFACE_(ISecurityInformation,IUnknown)
{
        /* IUnknown */
        STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;

	/* ISecurityInformation */
	STDMETHOD(GetObjectInformation)(THIS_ PSI_OBJECT_INFO) PURE;
	STDMETHOD(GetSecurity)(THIS_ SECURITY_INFORMATION,PSECURITY_DESCRIPTOR*,BOOL) PURE;
	STDMETHOD(SetSecurity)(THIS_ SECURITY_INFORMATION,PSECURITY_DESCRIPTOR) PURE;
	STDMETHOD(GetAccessRights)(THIS_ GUID*,DWORD,PSI_ACCESS*,ULONG*,ULONG*) PURE;
	STDMETHOD(MapGeneric)(THIS_ GUID*,UCHAR*,PSI_ACCESS*) PURE;
	STDMETHOD(GetInheritTypes)(THIS_ PSI_INHERIT_TYPE*,ULONG*) PURE;
	STDMETHOD(PropertySheetPageCallback)(THIS_ HWND,UINT,SI_PAGE_TYPE) PURE;
};
#undef INTERFACE

#undef INTERFACE
#define INTERFACE ISecurityInformation2
DECLARE_INTERFACE_(ISecurityInformation2,IUnknown)
{
        /* IUnknown */
        STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;

	/* ISecurityInformation2 */
	STDMETHOD(IsDaclCanonical)(THIS_ PACL) PURE;
	STDMETHOD(LookupSids)(THIS_ ULONG,PSID*,LPDATAOBJECT*) PURE;
};
#undef INTERFACE

#undef INTERFACE
#define INTERFACE IEffectivePermission
DECLARE_INTERFACE_(IEffectivePermission,IUnknown)
{
        /* IUnknown */
        STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;

	/* IEffectivePermission */
	STDMETHOD(GetEffectivePermission)(THIS_ GUID*,PSID,LPCWSTR,PSECURITY_DESCRIPTOR,POBJECT_TYPE_LIST*,ULONG*,PACCESS_MASK*,ULONG*) PURE;
};
#undef INTERFACE

#undef INTERFACE
#define INTERFACE ISecurityObjectTypeInfo
DECLARE_INTERFACE_(ISecurityObjectTypeInfo,IUnknown)
{
        /* IUnknown */
        STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;

	/* ISecurityObjectTypeInfo */
	STDMETHOD(GetInheritSource)(THIS_ SECURITY_INFORMATION,PACL,PINHERITED_FROM*) PURE;
};
#undef INTERFACE

HPROPSHEETPAGE WINAPI
CreateSecurityPage(LPSECURITYINFO psi);

BOOL WINAPI
EditSecurity(HWND hwndOwner, LPSECURITYINFO psi);

#ifdef __cplusplus
}
#endif
#endif /* __ACLUI_H */

/* EOF */
