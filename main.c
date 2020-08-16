#include <efi.h>
#include <efilib.h>
#include <efidef.h>

#include "efiConsoleControl.h"
#include "efiUgaDraw.h"

/**


 rEFIt License
===============

Copyright (c) 2006 Christoph Pfisterer
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

 * Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the
   distribution.

 * Neither the name of Christoph Pfisterer nor the names of the
   contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

static EFI_GUID ConsoleControlProtocolGuid = EFI_CONSOLE_CONTROL_PROTOCOL_GUID;
static EFI_CONSOLE_CONTROL_PROTOCOL *ConsoleControl = NULL;

static EFI_GUID UgaDrawProtocolGuid = EFI_UGA_DRAW_PROTOCOL_GUID;
static EFI_UGA_DRAW_PROTOCOL *UgaDraw = NULL;

static EFI_GUID GraphicsOutputProtocolGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
static EFI_GRAPHICS_OUTPUT_PROTOCOL *GraphicsOutput = NULL;

static BOOLEAN egHasGraphics = FALSE;
static UINTN egScreenWidth  = 800;
static UINTN egScreenHeight = 600;
// Make the necessary system calls to identify the current graphics mode.
// Stores the results in the file-global variables egScreenWidth,
// egScreenHeight, and egHasGraphics. The first two of these will be
// unchanged if neither GraphicsOutput nor UgaDraw is a valid pointer.
static VOID egDetermineScreenSize(VOID) {
  EFI_STATUS Status = EFI_SUCCESS;
  UINT32 UGAWidth, UGAHeight, UGADepth, UGARefreshRate;

  // get screen size
  egHasGraphics = FALSE;
  if (GraphicsOutput != NULL) {
    egScreenWidth = GraphicsOutput->Mode->Info->HorizontalResolution;
    egScreenHeight = GraphicsOutput->Mode->Info->VerticalResolution;
    egHasGraphics = TRUE;
  } else if (UgaDraw != NULL) {
    Status = UgaDraw->GetMode(UgaDraw, &UGAWidth, &UGAHeight, &UGADepth, &UGARefreshRate);
    if (EFI_ERROR(Status)) {
      UgaDraw = NULL;   // graphics not available
    } else {
      egScreenWidth  = UGAWidth;
      egScreenHeight = UGAHeight;
      egHasGraphics = TRUE;
    }
  }
} // static VOID egDetermineScreenSize()

VOID egInitScreen(VOID)
{
  Print(L"egInitScreen\n");
  EFI_STATUS Status = EFI_SUCCESS;

  // get protocols
  Status = LibLocateProtocol(&ConsoleControlProtocolGuid, (VOID **) &ConsoleControl);
  if (EFI_ERROR(Status))
    ConsoleControl = NULL;

  Status = LibLocateProtocol(&UgaDrawProtocolGuid, (VOID **) &UgaDraw);
  if (EFI_ERROR(Status))
    UgaDraw = NULL;

  Status = LibLocateProtocol(&GraphicsOutputProtocolGuid, (VOID **) &GraphicsOutput);
  if (EFI_ERROR(Status))
    GraphicsOutput = NULL;

  egDetermineScreenSize();
}

VOID egClearScreen()
{
  EFI_UGA_PIXEL FillColor;
  FillColor.Red   = 0xff;
  FillColor.Green = 0x0;
  FillColor.Blue  = 0x0;
  FillColor.Reserved = 0;

  if (GraphicsOutput != NULL) {
    // EFI_GRAPHICS_OUTPUT_BLT_PIXEL and EFI_UGA_PIXEL have the same
    // layout, and the header from TianoCore actually defines them
    // to be the same type.
    efi_call10(GraphicsOutput->Blt, GraphicsOutput, (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)&FillColor, EfiBltVideoFill,
                        0, 0, 0, 0, egScreenWidth, egScreenHeight, 0);
    Print(L"using GraphicsOutput\n");
  } else if (UgaDraw != NULL) {
    efi_call10(UgaDraw->Blt, UgaDraw, &FillColor, EfiUgaVideoFill, 0, 0, 0, 0, egScreenWidth, egScreenHeight, 0);
    Print(L"using Uga\n");
  }

}

EFI_STATUS
efi_main (EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
   InitializeLib(ImageHandle, SystemTable);
   Print(L"Hello, world!\n");

   egInitScreen();
   Print(L"Screen initialized %lux%lu\n", egScreenWidth, egScreenHeight);
   egClearScreen();

   return EFI_SUCCESS;
}
