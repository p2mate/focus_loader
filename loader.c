#include <dos.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>

typedef unsigned char u8;
typedef unsigned int u16;
typedef unsigned long u32;

#pragma pack ( 1 )
struct vbe_info_structure {
	char signature[4];
	u16 version;			// VBE version; high byte is major version, low byte is minor version
	u32 oem;			// segment:offset pointer to OEM
	u32 capabilities;		// bitfield that describes card capabilities
	u32 video_modes;		// segment:offset pointer to list of supported video modes
	u16 video_memory;		// amount of video memory in 64KB blocks
	u16 software_rev;		// software revision
	u32 vendor;			// segment:offset to card vendor string
	u32 product_name;		// segment:offset to card model name
	u32 product_rev;		// segment:offset pointer to product revision
	char reserved[222];		// reserved for future expansion
	char oem_data[256];		// OEM BIOSes store their strings in this area
};

#pragma pack ( 1 )
struct vbe_mode_info_structure {
	u16 attributes;		// deprecated, only bit 7 should be of interest to you, and it indicates the mode supports a linear frame buffer.
	u8 window_a;			// deprecated
	u8 window_b;			// deprecated
	u16 granularity;		// deprecated; used while calculating bank numbers
	u16 window_size;
	u16 segment_a;
	u16 segment_b;
	u32 win_func_ptr;		// deprecated; used to switch banks from protected mode without returning to real mode
	u16 pitch;			// number of bytes per horizontal line
	u16 width;			// width in pixels
	u16 height;			// height in pixels
	u8 w_char;			// unused...
	u8 y_char;			// ...
	u8 planes;
	u8 bpp;			// bits per pixel in this mode
	u8 banks;			// deprecated; total number of banks in this mode
	u8 memory_model;
	u8 bank_size;		// deprecated; size of a bank, almost always 64 KB but may be 16 KB...
	u8 image_pages;
	u8 reserved0;

	u8 red_mask;
	u8 red_position;
	u8 green_mask;
	u8 green_position;
	u8 blue_mask;
	u8 blue_position;
	u8 reserved_mask;
	u8 reserved_position;
	u8 direct_color_attributes;

	u32 framebuffer;		// physical address of the linear frame buffer; write here to draw to the screen
	u32 off_screen_mem_off;
	u16 off_screen_mem_size;	// size of memory in the framebuffer but not being displayed on the screen
	u8 reserved1[206];
};

static struct vbe_info_structure vbe_info;
static struct vbe_mode_info_structure vbe_mode;

typedef void (__interrupt __far *int_handler)();

static bool find_mode(u16 *mode)
{
	union REGS inregs, outregs;
	struct SREGS sregs;
	u16 far *modes;

	inregs.x.ax = 0x4f00;
	inregs.x.di = (unsigned int)&vbe_info;
	segread(&sregs);
	sregs.es = sregs.ds;
	int86x(0x10,&inregs, &outregs, &sregs);

	if (strncmp(vbe_info.signature, "VESA", 4) != 0) {
		fprintf(stderr, "VESA BIOS not found\n");
		return false;
	}

	modes = MK_FP(((unsigned int *)&vbe_info.video_modes)[1],
		      ((unsigned int *)&vbe_info.video_modes)[0]);

	while (*modes != 0xffff) {
                inregs.x.ax = 0x4f01;
                inregs.x.cx = *modes;
                inregs.x.di = (unsigned int)&vbe_mode;
                segread(&sregs);
                sregs.es = sregs.ds;
                int86x(0x10,&inregs, &outregs, &sregs);
		if (vbe_mode.width == 320 && vbe_mode.height == 240 && vbe_mode.bpp == 16) {
			*mode = *modes;
			return true;
		}
                modes++;
        }
	return false;
}

int main(int argc, char **argv)
{
	int err;
	int_handler far *chain_int10;
	u16 mode;
	extern void __interrupt __far int10_override(void);
	extern int_handler far *setup_int10(u16 mode);

	if (!find_mode(&mode)) {
		return 1;
	}

	chain_int10 = setup_int10(mode);
        *chain_int10 = _dos_getvect(0x10);

	_dos_setvect(0x10, int10_override);

	err = spawnl(P_WAIT, argv[1], NULL);

	_dos_setvect(0x10, *chain_int10);
	fprintf(stderr, "err: %d\n", err);

	return 0;
}
