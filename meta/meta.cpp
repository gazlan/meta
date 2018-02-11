/* ******************************************************************** **
** @@ Meta
** @  Copyrt :
** @  Author :
** @  Modify :
** @  Update :
** @  Notes  : Append <meta> & <link> HTML tags after <title> tag
** ******************************************************************** */

/* ******************************************************************** **
**                uses pre-compiled headers
** ******************************************************************** */

#include <stdafx.h>

#include "..\shared\mmf.h"
#include "..\shared\text.h"
#include "..\shared\vector.h"
#include "..\shared\vector_sorted.h"
#include "..\shared\file.h"
#include "..\shared\file_walker.h"
#include "..\shared\search_bmh.h"

/* ******************************************************************** **
** @@                   internal defines
** ******************************************************************** */

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#ifdef NDEBUG
#pragma optimize("gsy",on)
#pragma comment(linker,"/FILEALIGN:512 /MERGE:.rdata=.text /MERGE:.data=.text /SECTION:.text,EWR /IGNORE:4078")
#endif

#define BEGIN_OF_TITLE        "<title>"
#define END_OF_TITLE          "</title>"
#define EOL_WIN               "\n"
#define EOL_DOS               "\r\n"

struct TANDEM
{
   int   _iStart;
   int   _iFinish;
};

/* ******************************************************************** **
** @@                   internal prototypes
** ******************************************************************** */

/* ******************************************************************** **
** @@                   external global variables
** ******************************************************************** */

extern DWORD   dwKeepError = 0;

/* ******************************************************************** **
** @@                   static global variables
** ******************************************************************** */
                  
static MMF        MF;
                  
static TANDEM     _Title;

/* ******************************************************************** **
** @@                   real code
** ******************************************************************** */

/* ******************************************************************** **
** @@ IsDosEolType()
** @  Copyrt :
** @  Author :
** @  Modify :
** @  Update :
** @  Notes  :
** ******************************************************************** */

static bool IsDosEolType(const BYTE* const pBuf,DWORD dwSize)
{
   DWORD    dwCR_Cnt = 0; // '\n'
   DWORD    dwLF_Cnt = 0; // '\r'

   for (DWORD ii = 0; ii < dwSize; ++ii)
   {
      if (pBuf[ii] == '\n')
      {
         ++dwCR_Cnt;
      }

      if (pBuf[ii] == '\r')
      {
         ++dwLF_Cnt;
      }
   }

   if (dwCR_Cnt == dwLF_Cnt)
   {
      return true;
   }

   return false;
}

/* ******************************************************************** **
** @@ ForEach()
** @  Copyrt :
** @  Author :
** @  Modify :
** @  Update :
** @  Notes  :
** ******************************************************************** */

static void ForEach(const char* const pszFileName)
{
   memset(&_Title,  0,sizeof(TANDEM));

   char     pszBackName[_MAX_PATH];
   char     pszDrive   [_MAX_DRIVE];
   char     pszDir     [_MAX_DIR];
   char     pszFName   [_MAX_FNAME];
   char     pszExt     [_MAX_EXT];

   _splitpath(pszFileName,pszDrive,pszDir,pszFName,pszExt);
   _makepath( pszBackName,pszDrive,pszDir,pszFName,"BAK");

   CopyFile(pszFileName,pszBackName,FALSE);

   HANDLE   hFile = CreateFile(pszFileName,CREATE_ALWAYS,0);

   VERIFY(hFile != INVALID_HANDLE_VALUE);

   if (hFile == INVALID_HANDLE_VALUE)
   {
      // Error !
      CloseHandle(hFile);
      hFile = INVALID_HANDLE_VALUE;
      return;
   }

   if (!MF.OpenReadOnly(pszBackName))
   {
      // Error !
      CloseHandle(hFile);
      hFile = INVALID_HANDLE_VALUE;
      return;
   }

   BYTE*    pText  = MF.Buffer();
   DWORD    dwSize = MF.Size();

   ASSERT(pText);
   ASSERT(dwSize);

   if (IsBadReadPtr(pText,dwSize))
   {
      // Error !
      CloseHandle(hFile);
      hFile = INVALID_HANDLE_VALUE;
      return;
   }

   bool     bTitlePresent = false;

   // Title
   _Title._iStart = BMH_ISearch(pText,dwSize,(BYTE*)BEGIN_OF_TITLE,sizeof(BEGIN_OF_TITLE) - 1);

   if (_Title._iStart == -1)
   {
      printf(">> %s:\n[*] Warning: No <title> HTML tag found in the file.\n\n",pszFileName);
   }

   if (_Title._iStart != -1)
   {
      _Title._iFinish = BMH_ISearch(pText,dwSize,(BYTE*)END_OF_TITLE,sizeof(END_OF_TITLE) - 1);

      bTitlePresent = _Title._iFinish > _Title._iStart  ?  true  :  false;

      if (_Title._iFinish < _Title._iStart)
      {
         printf(">> %s:\n[!] Error: Incorrect HTML file.\nOccurrent </title> tag before <title> tag.\n",pszFileName);
         printf("</title> tag position %08X\n",_Title._iFinish);
         printf("<title> tag position  %08X\n\n",_Title._iStart);
      }
   }

   if (bTitlePresent)
   {
      bool  bDosEolType = IsDosEolType(pText,dwSize);

      // Skip Title
      _Title._iFinish += strlen(END_OF_TITLE);

      WriteBuffer(hFile,pText,_Title._iFinish);
      WriteBuffer(hFile,"\r\n",2);

      char     pszTemp[MAX_PATH * 3];

      if (bDosEolType)
      {
         sprintf(pszTemp,"<link rel=\"canonical\" href=\"%s%s\"/>\r\n",pszFName,pszExt);
      }
      else
      {
         sprintf(pszTemp,"<link rel=\"canonical\" href=\"%s%s\"/>\n",pszFName,pszExt);
      }

      WriteBuffer(hFile,pszTemp,strlen(pszTemp));

      if (bDosEolType)
      {
         sprintf(pszTemp,"<meta http-equiv=\"refresh\" content=\"0;URL='%s%s'\">\r\n",pszFName,pszExt);
      }
      else
      {
         sprintf(pszTemp,"<meta http-equiv=\"refresh\" content=\"0;URL='%s%s'\">\n",pszFName,pszExt);
      }

      WriteBuffer(hFile,pszTemp,strlen(pszTemp));
      
      WriteBuffer(hFile,pText + _Title._iFinish,dwSize - _Title._iFinish);
   }
   
   MF.Close();

   CloseHandle(hFile);
   hFile = INVALID_HANDLE_VALUE;
}

/* ******************************************************************** **
** @@ ShowHelp()
** @  Copyrt :
** @  Author :
** @  Modify :
** @  Update :
** @  Notes  :
** ******************************************************************** */

static void ShowHelp()
{
   const char  pszCopyright[] = "-*-   Meta 1.0   *   Copyright (c) Gazlan, 2015   -*-";
   const char  pszDescript [] = "Append <meta> & <link> HTML tags after <title> tag";
   const char  pszE_Mail   [] = "complains_n_suggestions direct to gazlan@yandex.ru";

   printf("%s\n\n",pszCopyright);
   printf("%s\n\n",pszDescript);
   printf("Usage: meta.com wildcards [ > LOG.TXT ]\n\n");
   printf("%s\n\n",pszE_Mail);
}

/* ******************************************************************** **
** @@ main()
** @  Copyrt :
** @  Author :
** @  Modify :
** @  Update :
** @  Notes  :
** ******************************************************************** */

int main(int argc,char**argv)
{
   if (argc != 2)
   {
      ShowHelp();
      return 0;
   }

   if (argc == 2)
   {
      if ((!strcmp(argv[1],"?")) || (!strcmp(argv[1],"/?")) || (!strcmp(argv[1],"-?")) || (!stricmp(argv[1],"/h")) || (!stricmp(argv[1],"-h")))
      {
         ShowHelp();
         return 0;
      }
   }

   char  pszMask[MAX_PATH + 1];

   strncpy(pszMask,argv[1],MAX_PATH);
   pszMask[MAX_PATH] = 0; // ASCIIZ

   char     pszDrive   [_MAX_DRIVE];
   char     pszDir     [_MAX_DIR];
   char     pszFName   [_MAX_FNAME];
   char     pszExt     [_MAX_EXT];

   _splitpath(pszMask,pszDrive,pszDir,pszFName,pszExt);

   char     pszSrchMask[MAX_PATH + 1];
   char     pszSrchPath[MAX_PATH + 1];

   strcpy(pszSrchMask,pszFName);
   strcat(pszSrchMask,pszExt);

   strcpy(pszSrchPath,pszDrive);
   strcat(pszSrchPath,pszDir);

   Walker      Visitor;

   Visitor.Init(ForEach,pszSrchMask,false); 
   Visitor.Run(*pszSrchPath  ?  pszSrchPath  :  ".");

   return 0;
}

/* ******************************************************************** **
** @@                   End of File
** ******************************************************************** */
