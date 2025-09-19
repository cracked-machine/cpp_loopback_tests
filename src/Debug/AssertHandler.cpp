// This file is for debug assertion handling and stack trace logging on windows builds

#include <Debug/AssertHandler.hpp>
#include <iomanip>
#include <spdlog/spdlog.h>
#include <sstream>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <dbghelp.h>
#include <windows.h>

namespace Debug {

void stack_trace( void )
{
  SPDLOG_CRITICAL( "=== Stack Trace ===" );

  HANDLE process = GetCurrentProcess();
  HANDLE thread = GetCurrentThread();

  // Initialize symbol handler
  SymSetOptions( SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS );
  if ( !SymInitialize( process, nullptr, TRUE ) )
  {
    SPDLOG_CRITICAL( "Failed to initialize symbol handler" );
    return;
  }

  CONTEXT context;
  memset( &context, 0, sizeof( CONTEXT ) );
  context.ContextFlags = CONTEXT_FULL;
  RtlCaptureContext( &context );

  DWORD image;
  STACKFRAME64 stackframe;
  ZeroMemory( &stackframe, sizeof( STACKFRAME64 ) );

#ifdef _M_IX86
  image = IMAGE_FILE_MACHINE_I386;
  stackframe.AddrPC.Offset = context.Eip;
  stackframe.AddrPC.Mode = AddrModeFlat;
  stackframe.AddrFrame.Offset = context.Ebp;
  stackframe.AddrFrame.Mode = AddrModeFlat;
  stackframe.AddrStack.Offset = context.Esp;
  stackframe.AddrStack.Mode = AddrModeFlat;
#elif _M_X64
  image = IMAGE_FILE_MACHINE_AMD64;
  stackframe.AddrPC.Offset = context.Rip;
  stackframe.AddrPC.Mode = AddrModeFlat;
  stackframe.AddrFrame.Offset = context.Rsp;
  stackframe.AddrFrame.Mode = AddrModeFlat;
  stackframe.AddrStack.Offset = context.Rsp;
  stackframe.AddrStack.Mode = AddrModeFlat;
#elif _M_IA64
  image = IMAGE_FILE_MACHINE_IA64;
  stackframe.AddrPC.Offset = context.StIIP;
  stackframe.AddrPC.Mode = AddrModeFlat;
  stackframe.AddrFrame.Offset = context.IntSp;
  stackframe.AddrFrame.Mode = AddrModeFlat;
  stackframe.AddrBStore.Offset = context.RsBSP;
  stackframe.AddrBStore.Mode = AddrModeFlat;
  stackframe.AddrStack.Offset = context.IntSp;
  stackframe.AddrStack.Mode = AddrModeFlat;
#endif // _M_IA64

  for ( size_t i = 0; i < 25; i++ )
  {

    BOOL result = StackWalk64( image,
                               process,
                               thread,
                               &stackframe,
                               &context,
                               NULL,
                               SymFunctionTableAccess64,
                               SymGetModuleBase64,
                               NULL );

    if ( !result ) { break; }

    char buffer[sizeof( SYMBOL_INFO ) + MAX_SYM_NAME * sizeof( TCHAR )];
    PSYMBOL_INFO symbol = (PSYMBOL_INFO)buffer;
    symbol->SizeOfStruct = sizeof( SYMBOL_INFO );
    symbol->MaxNameLen = MAX_SYM_NAME;

    DWORD64 displacement = 0;

    // Get symbol information
    if ( SymFromAddr( process, stackframe.AddrPC.Offset, &displacement, symbol ) )
    {

      // Get line information
      IMAGEHLP_LINE64 line;
      line.SizeOfStruct = sizeof( IMAGEHLP_LINE64 );
      DWORD lineDisplacement;

      // Get module information
      IMAGEHLP_MODULE64 moduleInfo;
      moduleInfo.SizeOfStruct = sizeof( IMAGEHLP_MODULE64 );
      bool hasModuleInfo = SymGetModuleInfo64( process, stackframe.AddrPC.Offset, &moduleInfo );

      if ( SymGetLineFromAddr64( process, stackframe.AddrPC.Offset, &lineDisplacement, &line ) )
      {
        // We have full symbol information
        SPDLOG_CRITICAL( "[{:2}] {} in {}:{}", i, symbol->Name, line.FileName, line.LineNumber );

        if ( hasModuleInfo ) { SPDLOG_CRITICAL( "    Module: {}", moduleInfo.ModuleName ); }
      }
      else
      {
        // We only have function name
        SPDLOG_CRITICAL( "[{:2}] {} (no line info)", i, symbol->Name );
      }
    }
    else
    {
      // No symbol information at all
      SPDLOG_CRITICAL( "[{:2}] Unknown function at {:016X}", i, stackframe.AddrPC.Offset );
    }
  }

  SymCleanup( process );
}

} // namespace Debug

#else // For non-Windows platforms, we can use a placeholder or implement a
      // different stack trace mechanism

namespace Debug {
void stack_trace( void )
{
  // Implement platform-specific stack trace for non-Windows platforms here
  std::cerr << "Stack trace not implemented for this platform\n";
}
} // namespace Debug

#endif // For non-Windows platforms
