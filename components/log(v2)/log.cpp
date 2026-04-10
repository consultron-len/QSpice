// Automatically generated C++ file on Sun Mar 4 2026
//
// To build with Digital Mars C++ Compiler:
//
//    dmc -mn -WD -o log.cpp kernel32.lib
/* ========================================
 *
 * Copyright CONSULTRON, 2026
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONSULTRON allows PUBLIC use of the file.
 * CONSULTRON provides NO WARRANTY expressed or implied.
 *
 * ========================================
?  1.0.0 - Initial code.  Improvement on logger V2 code.
?  Supports:
?     - A single DLL can support up tp 6 inputs.  An input prarameter (in_cnt) informs the number of inputs to be logged.
?     - The InstanceName is prepended to the log file name requested.
?     - A list of logfile names requested is created at the beginning of the sim.  A Stepped run improvement.
?     - The requested logfile name is checked against the list for a previous step run.  If found, the file appends.  If not found, the file is created.
?  1.1.0 - Very minor mostly non-functional changes.  Changes for consistency. Thanks RDunn
?     - Change: data['x'] reference to data['x'].d to force a cast to double.
?     - Change: %f to %.12f printf specifier to go from 6-decimal precision to 12-decimal precision.
?     - Change: Improved the var "it" to cast to an iterator class.
?  1.2.0 - Comment updates to instruct if more than 6 inputs are desired.  Thanks RDunn
?  2.0.0 - Add: argument for data output format.  Thanks RDunn
?     - Add: Selectable argument for file output format.   Only .bin, .csv and .txt are allowed.
?        - file format string appended to end of logfile.
?     - Change: header arg to char *.  Use stricmp() on arg looking for "YES".
?  2.1.0 - Change: header arg to bool.  Use global equate false=NO, true=YES.  Thanks RDunn
?     - Add: More user-selectable inputs to 10.
*/

#define PROGRAM_NAME    "log"
#define PROGRAM_V_MAJOR "2"
#define PROGRAM_V_MINOR "1"   // up to 10-input
#define PROGRAM_INFO    PROGRAM_NAME " v" PROGRAM_V_MAJOR "." PROGRAM_V_MINOR

#include <malloc.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <algorithm>
using namespace std;

extern "C" __declspec(dllexport) void (*Display)(const char *format, ...)       = 0; // works like printf()
extern "C" __declspec(dllexport) void (*EXIT)(const char *format, ...)          = 0; // print message like printf() but exit(0) afterward
extern "C" __declspec(dllexport) const double *DegreesC                         = 0; // pointer to current circuit temperature
extern "C" __declspec(dllexport) const int *StepNumber                          = 0; // pointer to current step number
extern "C" __declspec(dllexport) const int *NumberSteps                         = 0; // pointer to estimated number of steps
extern "C" __declspec(dllexport) const char* const *InstanceName                = 0; // pointer to address of instance name
extern "C" __declspec(dllexport) const char *QUX                                = 0; // path to QUX.exe
extern "C" __declspec(dllexport) const bool *ForKeeps                           = 0; // pointer to whether being evaluated non-hypothetically
extern "C" __declspec(dllexport) const bool *HoldICs                            = 0; // pointer to whether instance initial conditions are being held
extern "C" __declspec(dllexport) const void *GUI_HWND                           = 0; // pointer to Window handle of QUX.exe
extern "C" __declspec(dllexport) const double *CKTtime                          = 0;
extern "C" __declspec(dllexport) const double *CKTdelta                         = 0;
extern "C" __declspec(dllexport) const int *IntegrationOrder                    = 0;
extern "C" __declspec(dllexport) const char *InstallDirectory                   = 0;
extern "C" __declspec(dllexport) double (*EngAtof)(const char **string)         = 0;
extern "C" __declspec(dllexport) const char *(*BinaryFormat)(unsigned int data)                          = 0; // BinaryFormat(0x1C) returns "0b00011100"
extern "C" __declspec(dllexport) const char *(*EngFormat   )(double x, const char *units, int numDgts)   = 0; // EngFormat(1e-6, "s", 6) returns "1�s"
extern "C" __declspec(dllexport) int (*DFFT)(struct sComplex *u, bool inv, unsigned int N, double scale) = 0; // Discrete Fast Fourier Transform
extern "C" __declspec(dllexport) void (*bzero)(void *ptr, unsigned int count)   = 0;

union uData {
   bool b;
   char c;
   unsigned char uc;
   short s;
   unsigned short us;
   int i;
   unsigned int ui;
   float f;
   double d;
   long long int i64;
   unsigned long long int ui64;
   char *str;
   unsigned char *bytes;
};

// int DllMain() must exist and return 1 for a process to load the .DLL
// See https://docs.microsoft.com/en-us/windows/win32/dlls/dllmain for more information.
int __stdcall DllMain(void *module, unsigned int reason, void *reserved) { return 1; }


#define INPUT_CNT_MAX   10
/* If you need more inputs:
   - increase this number first.
   - Create (or copy) a new symbol with the # of inputs in the name.
   - Add the additional inputs to the symbol.  Note: Pin order is important.
   - The code below will automatically add the new columns (and header name fields if selected).
*/

// Binary file format version: major<<16 | minor
#define BIN_FILE_VERSION ((unsigned int)((2u << 16) | 0u))   // 2.0

// #undef pin names lest they collide with names in any header file(s) you might include.
#undef data0
#undef data1
#undef data2
#undef data3
#undef data4
#undef data5
#undef data6
#undef data7
#undef data8
#undef data9
#undef data10
#undef trig

//#define DEBUG
#if defined(DEBUG)
   #define DDisplay  Display
#else
   #define DDisplay
#endif

#pragma optimize(speed)    // suggested by RDunn

using namespace std;

/******/
enum
{  // Use lowercase substitution
   tt_RE = ('e'<<8)+'r',   // rising edge
   tt_FE = ('e'<<8)+'f',   // falling edge
   tt_WH = ('h'<<8)+'w',   // Windowed @ 1V
   tt_WL = ('l'<<8)+'w',   // Windowed @ 0V
} etrig;

/**************************************************************/
struct sLOGGER {
  // declare the structure here

  bool trig_prev; // previous trigger value
  FILE *filep;     // file struct
  bool ffmt_bin;     // selected file format true=binary false=csv or txt
  enum etrig trigtype;  // selected trigger type.
};


vector<string> fnlist; // vector of filename strings
string filename(""); // filename
/**************************************************************/
extern "C" __declspec(dllexport) void log(sLOGGER **opaque, double t, union uData *data) {

   bool         trig    = data[0].b  ; // input
/******** Commented out.  Not used.  Here as placeholders.  data[] vars are called directly.
double dat[INPUT_CNT_MAX+1];  // Not used here
********/
   const char *logfile  = data[INPUT_CNT_MAX+1].str; // input parameter
   bool        header   = data[INPUT_CNT_MAX+2].b;  // input parameter
   int         in_cnt   = data[INPUT_CNT_MAX+3].i;   // input parameter
   const char *dfmt     = data[INPUT_CNT_MAX+4].str;   // input parameter
   const char *ffmt     = data[INPUT_CNT_MAX+5].str;   // input parameter
   const char *trigtype = data[INPUT_CNT_MAX+6].str;   // input parameter

  sLOGGER *inst = *opaque;

  if (!*opaque)
  {
  bool started = false;
  char *mode = "  ";

/********  Let's do some quick bounds checking */
    if(in_cnt > INPUT_CNT_MAX)
    {  //Input count exceeded.
      Display("Input count exceeded.  Max allowed = %u.   Simulation stopped.\n",INPUT_CNT_MAX);
      exit(4);   // exit sim
    }

    /*** Compute logfile ***/
    inst = *opaque = (sLOGGER *)calloc(1, sizeof(sLOGGER));
    string newfn = (*InstanceName);
    newfn.append(" ").append(logfile).append(ffmt);

   /*** Find if the logfile name already exists in this sim run ***/
    vector<string>::iterator it = find(fnlist.begin(), fnlist.end(), newfn);
    if(it==fnlist.end())
    { // newfilename not found on list
      mode = "w";      // Delete (it already exists) and go to write.
      started = true;
      fnlist.push_back(newfn);   // Push the logfile name on the list
    }
    else
    { // newfilename found on list
      mode = "a";      // Append write.
    }

    /*** Determine the file format to use ***/
    if(0 == stricmp(ffmt,".bin"))
    {
      inst->ffmt_bin = true;
      strcat(mode, "b");      // append the "b" binary mode
DDisplay("binary format selected\n");
    }
DDisplay("mode=%s\n",mode);

    inst->filep      = fopen(newfn.c_str(), mode); // open the logfile

    if (inst->filep == NULL)
    {    // Couldn't open logfile.
      Display("Could not open or create \"%s\".   Simulation stopped.\n",newfn);
      exit(3);   // exit sim if you cannot open the requested file
    }

    if(started == true)
    {
         if(inst->ffmt_bin)
         {     // binary header format courtesy of RDunn
            // Write binary file header
            unsigned int version = BIN_FILE_VERSION;
            fwrite(&version, sizeof(version), 1, inst->filep);   // uint32_t version
            fwrite(&in_cnt, sizeof(in_cnt), 1, inst->filep);     // int      in_cnt
            fwrite(&header, sizeof(header), 1, inst->filep);           // bool     header
DDisplay("version=%X\n",version);
         }
         else
         {
            /*** Test if the header is requested ***/
            if(header)    // test if header is requested
            {  // Yes.  fprint the header.
DDisplay("fprintf header ...\n");
               {     // csv or txt format
                  fprintf(inst->filep, "t");
                  for(char i=1; i<=in_cnt; i++)  { fprintf(inst->filep, ",data%d", i);}
                  fprintf(inst->filep, "\n");
               }
            }
         }
    }
    /*** Determine the trigger type ***/
    inst->trigtype = (enum etrig)((*(short *)trigtype) | 0x2020);   // convert to lower-case
DDisplay("e_trig = %X    etrig = %X\n",inst->trigtype, tt_RE);
  }

  // Implement module evaluation code here:
  bool printdata = false;
  switch((short )inst->trigtype)
  {
  case tt_RE:
      if ((trig == 1) && (inst->trig_prev == 0))   // Rising edge trigger detected.
         printdata = true;
      break;
  case tt_FE:
     if ((trig == 0) && (inst->trig_prev == 1))    // falling edge trigger detected.
         printdata = true;
     break;
  case tt_WH:
     if (trig == 1)        // Windowed level @ 1V
         printdata = true;
     break;
  case tt_WL:
     if (trig == 0)        // Windowed level @ 0V
         printdata = true;
     break;
  }

   /*** data logging criteria met.  fprintf the data to the logfile ***/
  if(printdata == true)
  {
     if(inst->ffmt_bin)
     {     // binary format courtesy of RDunn
     // write without extra copy to a record buffer
         fwrite(&t, sizeof(double), 1, inst->filep);
         fwrite(data + 1, sizeof(double), in_cnt, inst->filep);
     }
     else
     {     // csv or txt format
         fprintf(inst->filep, dfmt, t);  // push time
         for(int i=1; i<=in_cnt; i++)
         {
            fputc(',', inst->filep);   // push a comma to separate from previous column.
            fprintf(inst->filep, dfmt, data[i].d); // write data[i]
         }
         fputc('\n', inst->filep);   // push a linefeed to separate data events
     }
  }
  inst->trig_prev = trig;
}

/**************************************************************/
extern "C" __declspec(dllexport) void Destroy(sLOGGER *inst) {
   fclose(inst->filep);    // Close the opened file
   free(inst);             // Free the instance struct
}
