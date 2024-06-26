/**********************************/
/*                                */
/*  Copyright 2000, David Grant   */
/*                                */
/*  see LICENSE for more details  */
/*                                */
/**********************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*#define TRACER_OFF*/
#include "coldfire.h"



/* s32word - (msb) 3 2 1 0 (lsb)
 * Big endian:
 *   b3 b2 b1 b0 
 * Little endian:
 *   b0 b1 b2 b3
 */


/* External defs that need to be initialized */
struct _memory_core memory_core;
//struct _SR *SRBits = (struct _SR *)&memory_core.sr;

/* memory core copy with values used when reset */
static struct _memory_core memory_core_reset_values;

TRACER_DEFAULT_CHANNEL(memory);


static struct _memory_module *memory_module = NULL;
static int memory_module_count=0;
static int memory_module_count_max=0;

/* Make a list of pointers to segments.  Note: we don't make a list of segments
 *  directly, because some segments will want to save a pointer to themselves,
 *  and they can't do that if we're realloc()ing the list as we add more items.
 */
struct _memory_segment_list_item {
	struct _memory_segment *seg;
};
static struct _memory_segment_list_item *memory_segment_list = NULL;
static int memory_segment_count=0;
static int memory_segment_count_max=0;



void memory_module_register(char *name, 
		void (*setup)(struct _memory_segment *s))
{
	if(memory_module_count == memory_module_count_max) {
		memory_module_count_max += 4;
		memory_module = realloc(memory_module,
					sizeof(struct _memory_module) * 
						memory_module_count_max);
	}
	memory_module[memory_module_count].name = strdup(name);
	memory_module[memory_module_count].setup = setup;
	memory_module_count++;	
}


static u32 zero_register = 0;
void memory_module_setup_segment(char *module_name, char *data)
{
	struct _memory_segment *s;
	struct _memory_segment_list_item *i;
	int argc;
	char *argv[16];
	int x;
	char movable = 1;
	
	struct _reg{
		char *name;
		u32 *ptr;
	} regs[] = {
		{ "mbar", &memory_core.mbar },
		{ "rombar", &memory_core.rombar },
		{ "rambar", &memory_core.rambar },
		{ NULL, NULL }};

	
	/* Setup the memory segment */
	if(memory_segment_count == memory_segment_count_max) {
		memory_segment_count_max += 4;
		memory_segment_list = realloc(memory_segment_list,
					sizeof(struct _memory_segment_list_item) * 
						memory_segment_count_max);
	}
	i = &memory_segment_list[memory_segment_count];
	s = malloc(sizeof(struct _memory_segment));
	i->seg = s;
	memset(s, 0, sizeof(struct _memory_segment));

	/* Split the data */
	TRACE("data=[%s]\n", data);

	argc = arg_split_chars(argv, data, 15, " \t,=");

	TRACE("   split into %d args\n", argc);

	for(x=0;x<argc;x+=2) {
		if(strcasecmp(argv[x], "name") == 0) {
			/* deal with the name */
			s->name = strdup(argv[x+1]);
			TRACE("name=[%s]\n", s->name);
		} else if(strcasecmp(argv[x], "base") == 0) {
			int b_argc;
			char *b_argv[4];
			int i;
			/* now the base register */
			TRACE("base=[%s]\n", argv[x+1]);
			b_argc = arg_split_chars(b_argv, argv[x+1], 4, "+");
			TRACE("split into %d args\n", b_argc);
			for(i=0;i<b_argc;i++) {
				struct _reg *r = regs;
				if(b_argv[i][0] == '0') {
					TRACE("base address=[%s]\n", b_argv[i]);
					sscanf(b_argv[i], "%x", &s->base);
					continue;
				}
				for(r=regs; r->name != NULL; r++) {
					if(strcasecmp(r->name, b_argv[i])==0){
						TRACE("base register=[%s]\n", b_argv[i]);
						s->base_register = r->ptr;
						break;
					}
				}
			}
		} else if(strcasecmp(argv[x], "len") == 0) {
			TRACE("len=[%s]\n", argv[x+1]);
			/* turn the length into an access mask, if len is unspecified, then
			 * we rely on the module to set the mask correctly */
			sscanf(argv[x+1], "%x", &s->mask);
			/* The mask is the inverted length 
			 *   len 0x100 ==  mask FFFFFF00 */
			s->mask = ~(s->mask - 1);
		} else if(strcasecmp(argv[x], "int") == 0) {
			sscanf(argv[x+1], "%hu", &s->interrupt_line);
		} else if(strcasecmp(argv[x], "code") == 0) {
			int i;
			s->code_len = strlen(argv[x+1]) / 2;
			s->code = malloc(s->code_len);
			TRACE("code length=%d\n", s->code_len);
			for(i=0;i<s->code_len; i++) {
				char val[3] = { argv[x+1][(i*2)], 
						argv[x+1][(i*2)+1], 0 };
				int v;
				sscanf(val, "%x", &v);
				s->code[i] = v;
				TRACE(" code[%d] = 0x%02x\n", i, v);
			}
		}
	}
	
	printf("%s", s->name);
	/* Find the module, and run the module setup */
	for(x=0;x<memory_module_count;x++) {
		if(strcasecmp(memory_module[x].name, module_name) == 0) {
			memory_module[x].setup(s);
			break;
		}
	}
	if(x==memory_module_count) {
		/* Not found */
		printf("Could not find module for [%s]\n", module_name);
	}
	
	/* See if everything above registerd a base address register, if not
	 * then give it the zero address */
	if(s->base_register == NULL) {
		s->base_register = &zero_register;
		movable=0;
	}
	
/*	printf("0x%08x -> 0x%08x  %s\n",
			*s->base_register + s->base, 
			*s->base_register + s->base + ~s->mask,
			movable ? "(movable)" : "");
*/	
	printf("  ");
	fflush(stdout);
	memory_segment_count++;
}

struct _memory_segment *memory_find_segment_for(u32 offset)
{
	int x;
	struct _memory_segment_list_item *i;
	TRACE("looing for a segment for offset=0x%08x\n", offset);
	for(x=0,i=&memory_segment_list[0];x<memory_segment_count;x++,i++) {
		/* FIXME: optimize this by pre-calculation */
		struct _memory_segment *s = i->seg;
		register u32 b = *s->base_register + s->base;
		TRACE("%d: name=%s, base_reg=0x%08x, base=0x%08x, mask=0x%08x\n",
				x, s->name, *s->base_register, 
				s->base, s->mask);
		if( (offset & s->mask) == b) {
			TRACE("   match.\n");
			return s;
		}
	}
	TRACE("no match.\n");
	return NULL;
}


static void memory_core_reset(void)
{
	/* resetore default values */
	memory_core.pc_instruction_begin = 0x0;
	SET_SR(memory_core_reset_values.sr_reset);
	memory_core.vbr = memory_core_reset_values.vbr;
	memory_core.mbar = memory_core_reset_values.mbar;
	memory_core.rambar = memory_core_reset_values.rambar;
	memory_core.rombar = memory_core_reset_values.rombar;
	memory_core.pc = memory_core_reset_values.pc;
	memory_core.a[7] = memory_core_reset_values.a[7] & 0xFFFFFFF0;
}


void memory_core_set_reset_values(char *s)
{
	int argc;
	char *argv[16];
	int x;
	static struct _reg{
		char *name;
		u32 *ptr;
	} regs[] = {
		{ "mbar", &memory_core_reset_values.mbar },
		{ "rombar", &memory_core_reset_values.rombar },
		{ "rambar", &memory_core_reset_values.rambar },
		{ "vbr", &memory_core_reset_values.vbr },
		{ "sr", &memory_core_reset_values.sr_reset },
		{ "pc", &memory_core_reset_values.pc },
		{ "sp", &memory_core_reset_values.a[7] },
		{ NULL, NULL }};
		
	argc = arg_split_chars(argv, s, 15, " \t,=");
	for(x=0;x<argc;x+=2) {
		u32 value;
		int i;
		sscanf(argv[x+1], "%x", &value);
		for(i=0; regs[i].name != NULL; i++) {
			if(strcasecmp(argv[x], regs[i].name) == 0) {
				TRACE("Setting reset value of %s to 0x%08x\n",
						regs[i].name, value);
				*(regs[i].ptr) = value;
			}
		}
	}
}



void memory_reset(void)
{
	int x;
	struct _memory_segment_list_item *i;


	TRACE("resetting memory core before init.\n");
	memory_core_reset();

	TRACE("loading vector table\n");
	/* Write the rombar to the entries in the vector table  */
	for(x=0;x<256;x++)
		Memory_Stor(32, (x*4)+memory_core.vbr, memory_core.rombar);

	
	TRACE("resetting all segments\n");
	for(x=0,i=&memory_segment_list[0];x<memory_segment_count;x++,i++) {
		struct _memory_segment *s = i->seg;
		if(s->reset) s->reset(s);
	}

#if 0
	TRACE("finding memory length\n");
	/* Detect the length of the memory */
	x += 0x100;
	while(1) {
		x += 0x100;
		if(!memory_seek(x)) {
			break;
		}
	}
	TRACE("length=0x%08x\n", x);
	/* Start A7 pointing aligned at the end of memory */
	memory_core.a[7]=(x & 0xFFFFFFF0) -4;
#endif

	TRACE("storing PC and SR\n");
	/* Fill out the values of the PC and the SP in the vector table */
	Memory_Stor(32, memory_core.vbr, memory_core.a[7]);
	Memory_Stor(32, memory_core.vbr+4, memory_core.pc);
}

void Memory_Init(void)
{
	/* First reset the core, this just sets the core regsters, useful
	 * because the modules depend on some of the core registers, like
	 * the rombar and the mbar (just so they print properly) */
	memory_core_reset();
	
	printf("Loading memory modules...\n");
	ram_init();
	timer_5206_init();
	serial_5206_init();
	sim_5206_init();
	sim_5307_init();
	isa_init();
}


void Memory_DeInit(void)
{
	int x;
	struct _memory_segment_list_item *i;
	for(x=0,i=&memory_segment_list[0];x<memory_segment_count;x++,i++) {
		struct _memory_segment *s = i->seg;
		s->fini(s);
		if(s->code) free(s->code);
		free(s);
			
	}
	free(memory_segment_list);
}

/* See if we can seek to address at offset */
char memory_seek(u32 offset)
{
	struct _memory_segment *s;
	
	TRACE("Testing Offset=0x%08x\n", offset);
	s = memory_find_segment_for(offset);
	
	return (s==NULL) ? 0 : 1;
}

/*    Desc: Retrieves a value from the memory
 * Returns: 1 if successful, 0 if something bad happened 
 *   Notes: This always first retrieves a s32 into *Result, then it trims it down
 */
	void rd_dump_registers(u32 cpc, u32 csr);
char Memory_Retr(u32 *Result, s16 Size, s32 Offset)
{
	struct _memory_segment *s;
	u32 base_offset;
	
	TRACE("Size=%d, Offset=0x%08x\n", Size, Offset);

	s = memory_find_segment_for(Offset);
	if(s) {
		base_offset = (u32)Offset - 
					(*s->base_register + s->base);
		/* base_offset = (u32)Offset & ~(s->base); */
		return s->read(s, Result, Size, base_offset);
	}

	/* Coulnd't find it in the tables */
	printf("retr retr failed for size=%d, offset=0x%08x\n", Size, Offset);
	rd_dump_registers(memory_core.pc, GET_SR());

	exception_do_exception(2);
	return 0;
}


char Memory_Stor(s16 Size, s32 Offset, u32 Value)
{
	struct _memory_segment *s;
	u32 base_offset;
	TRACE("Size=%d, Offset=0x%08x, Value=0x%08x\n", Size, Offset, Value);
	/* Value will be in whatever endianness the computer is */

	s = memory_find_segment_for(Offset);
	if(s) {
		base_offset = (u32)Offset - 
			(*s->base_register + s->base);
		/* base_offset = (u32)Offset & ~(s->base); */
		return s->write(s, Size, base_offset, Value);
	}
	
	/* Exception 2, access error */
	printf("retr stor failed for size=%d, offset=0x%08x\n", Size, Offset);
	rd_dump_registers(memory_core.pc, GET_SR());
	exception_do_exception(2);
	return 0;
}

char Memory_RetrFromPC(u32 *Result, s16 Size)
{
	char ReturnValue;
	switch(Size) {
	case 32:
		ReturnValue = Memory_Retr(Result, 32, memory_core.pc);
		memory_core.pc+=4;
		return ReturnValue;
	case 16:
		ReturnValue = Memory_Retr(Result, 16, memory_core.pc);
		memory_core.pc+=2;
		return ReturnValue;
	case 8:
		memory_core.pc+=1;	/* Skip the first byte */
		ReturnValue = Memory_Retr(Result, 8, memory_core.pc);
		memory_core.pc+=1;	/* Go past the byte we just read */
		return ReturnValue;
	}
	return 0;
}

void memory_update(void)
{
	int x;
	struct _memory_segment_list_item *i;
	for(x=0, i=&memory_segment_list[0];x<memory_segment_count;x++,i++) {
		struct _memory_segment *s = i->seg;
		if(s->update) s->update(s);
	}
}


void memory_dump_segments(void)
{
	int x;
	struct _memory_segment_list_item *i;
	for(x=0, i=&memory_segment_list[0];x<memory_segment_count;x++,i++) {
		struct _memory_segment *s = i->seg;
		printf("%s: @%p, base_reg=%p, base=0x%08x, data=%p\n",
				s->name, s, s->base_register, s->base, s->data);
	}
}


