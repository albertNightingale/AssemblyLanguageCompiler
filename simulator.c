/*
 * Author: Daniel Kopta
 * Updated by: Erin Parker
 * CS 4400, University of Utah
 *
 * Simulator handout
 * A simple x86-like processor simulator.
 * Read in a binary file that encodes instructions to execute.
 * Simulate a processor by executing instructions one at a time and appropriately 
 * updating register and memory contents.
 *
 * Some code and pseudo code has been provided as a starting point.
 * Edited by: Albert Liu
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "instruction.h"

// Forward declarations for helper functions
unsigned int get_file_size(int file_descriptor);
unsigned int* load_file(int file_descriptor, unsigned int size);
instruction_t* decode_instructions(unsigned int* bytes, unsigned int num_instructions);
unsigned int execute_instruction(unsigned int program_counter, instruction_t* instructions, 
				 int* registers, unsigned char* memory);
void print_instructions(instruction_t* instructions, unsigned int num_instructions);
void error_exit(const char* message);

// 17 registers
#define NUM_REGS 17
// 1024-byte stack
#define STACK_SIZE 1024

int main(int argc, char** argv)
{
  // Make sure we have enough arguments
  if(argc < 2)
    error_exit("must provide an argument specifying a binary file to execute");

  // Open the binary file
  int file_descriptor = open(argv[1], O_RDONLY);
  if (file_descriptor == -1) 
    error_exit("unable to open input file");

  // Get the size of the file
  unsigned int file_size = get_file_size(file_descriptor);
  // Make sure the file size is a multiple of 4 bytes
  // since machine code instructions are 4 bytes each
  if(file_size % 4 != 0)
    error_exit("invalid input file");

  // Load the file into memory
  // We use an unsigned int array to represent the raw bytes
  // We could use any 4-byte integer type
  unsigned int* instruction_bytes = load_file(file_descriptor, file_size);  // bytes of instructions
  close(file_descriptor);

  unsigned int num_instructions = file_size / 4; // each instruction is 4 bytes


  /****************************************/
  /**** Begin code to modify/implement ****/
  /****************************************/

  // Allocate and decode instructions (left for you to fill in)
  instruction_t* instructions = decode_instructions(instruction_bytes, num_instructions);

  // Optionally print the decoded instructions for debugging
  // Will not work until you implement decode_instructions
  // Do not call this function in your submitted final version
  
  // print_instructions(instructions, num_instructions);

  // Once you have completed Part 1 (decoding instructions), uncomment the below block

  // Allocate and initialize registers
  int* registers = (int*)malloc(sizeof(int) * NUM_REGS);
  // initialize register values
  for (int idx = 0; idx < NUM_REGS; idx++)
  {
    if (idx != 6)
    {
      registers[idx] = 0;
    }
    else
    {
      registers[idx] = 1024;
    }
  }

  // Stack memory is byte-addressed, so it must be a 1-byte type
  // allocate the stack memory. Do not assign to NULL.
  unsigned char* memory = (unsigned char*)malloc(1024);
  for (int idx = 0; idx < 1024; idx ++)
  {
      memory[idx] = 0;
  }

  // Run the simulation
  unsigned int program_counter = 0;

  // printf("%s %d %d\n", "continue", program_counter, num_instructions);

  // program_counter is a byte address, so we must multiply num_instructions by 4 
  // to get the address past the last instructions

  while(program_counter <= num_instructions * 4) 
  {
    program_counter = execute_instruction(program_counter, instructions, registers, memory);

    // printf ("pc: %d, nis: %d\n", program_counter, num_instructions);
  }
  
  return 0;
}

/*
 * Decodes the array of raw instruction bytes into an array of instruction_t
 * Each raw instruction is encoded as a 4-byte unsigned int
*/
instruction_t* decode_instructions(unsigned int* bytes, unsigned int num_instructions)
{
  instruction_t* retval = (instruction_t*) malloc(num_instructions * sizeof(instruction_t));

  int i;
  for(i = 0; i < num_instructions; i++) {

    unsigned int byte = bytes[i];
    unsigned char part1 = (byte & 0b11111000000000000000000000000000) >> 27; 
    unsigned char part2 = (byte & 0b00000111110000000000000000000000) >> 22; 
    unsigned char part3 = (byte & 0b00000000001111100000000000000000) >> 17; 
    int16_t part4 = (byte & 0b00000000000000001111111111111111); 

    // printf("op: %u, reg1: %u, reg2: %u, imm: %u\n", part1, part2, part3, part4);
    retval[i] = (instruction_t) { .opcode = part1, .first_register = part2, .second_register = part3, .immediate = part4 };
  }

  return retval;
}


/*
 * Executes a single instruction and returns the next program counter
*/
unsigned int execute_instruction(unsigned int program_counter, instruction_t* instructions, int* registers, unsigned char* memory)
{
  // program_counter is a byte address, but instructions are 4 bytes each
  // divide by 4 to get the index into the instructions array
  instruction_t instr = instructions[program_counter / 4];
  
  /*
  printf("pc: %d, op: %d, reg1: %d, reg2: %d, imm: %d\n",       
    program_counter, instr.opcode, instr.first_register,
    instr.second_register, instr.immediate);
  */

  switch(instr.opcode)
  {
    case subl: // subtract immediate from register
      registers[instr.first_register] = registers[instr.first_register] - instr.immediate;
      break;
    case addl_reg_reg: // add register to register
      registers[instr.second_register] = registers[instr.first_register] + registers[instr.second_register];
      break;
    case addl_imm_reg:  // add immediate to register
      registers[instr.first_register] = registers[instr.first_register] + instr.immediate;
      break;
    case imull: // multiple first register by second register
      registers[instr.second_register] = registers[instr.first_register] * registers[instr.second_register];
      break; 
    case shrl:  // shift the value of first register to the right by 1;
      registers[instr.first_register] = ((unsigned int) registers[instr.first_register]) >> 1;
      break;
    case movl_reg_reg: // move from first register to second register
      registers[instr.second_register] = registers[instr.first_register];
      break;
    case movl_deref_reg: // mov OFFSET(start), destination 
    {
      unsigned int memoryAdd = registers[instr.first_register] + instr.immediate;
      unsigned int byte1 = memory[memoryAdd];
      unsigned int byte2 = (memory[memoryAdd + 1]) << 8;
      unsigned int byte3 = (memory[memoryAdd + 2]) << 16;
      unsigned int byte4 = (memory[memoryAdd + 3]) << 24;
      registers[instr.second_register] = byte1 + byte2 + byte3 + byte4; 
      // printf("byte2 : %d\n", byte2);
      // printf("in register %d: \n", registers[instr.second_register]);
      break;
    }
    case movl_reg_deref: // mov start, OFFSET(destination)
    {
      unsigned int memoryAdd = registers[instr.second_register] + instr.immediate;
      memory[memoryAdd] = registers[instr.first_register] & 0x000000FF;
      memory[memoryAdd + 1] = (registers[instr.first_register] & 0x0000FF00) >> 8;
      memory[memoryAdd + 2] = (registers[instr.first_register] & 0x00FF0000) >> 16;
      memory[memoryAdd + 3] = (registers[instr.first_register] & 0xFF000000) >> 24;
      // printf("memory %d %d %d %d: \n", 
      //    memory[memoryAdd], memory[memoryAdd + 1], memory[memoryAdd + 2], memory[memoryAdd + 3]);
      break;
    }
    case movl_imm_reg: // move immediate to register
      registers[instr.first_register] = instr.immediate;
      break;
    case cmpl: { // compare two registers
      // need to use braces here, reason see link below
      // https://stackoverflow.com/questions/22419790/c-error-expected-expression-before-int
      int number = registers[instr.second_register] - registers[instr.first_register];

      // Carry Flag
      int CF = (unsigned int) registers[instr.second_register] < (unsigned int) registers[instr.first_register];  
      // printf("carryflag: %d ", CF);
      int ZF = (number == 0) << 6;  // Zero Flag
      // printf("zeroflag: %d ", ZF);
      // shift number left by 31 to determine the significant bit. 
      int SF = (number < 0) << 7; // Sign Flag
      // printf("signflag: %d\n", SF);
      int OF = 0; 
      if (registers[instr.second_register] < 0 && registers[instr.first_register] > 0) 
      { // negative - positive and result in positive
        OF = (number > 0) << 11;
      }
      else if (registers[instr.second_register] > 0 && registers[instr.first_register] < 0) 
      { // positive - negative and result in negative
        OF = (number < 0) << 11;
      }
      registers[16] = CF + ZF + SF + OF;
      /*
      printf("\n\n");
      printf("CF %d: \n", CF);
      printf("ZF %d: \n", ZF);
      printf("SF %d: \n", SF);
      printf("OF %d: \n", OF);
      printf("logic: %d\n", registers[instr.second_register] < 0 && registers[instr.first_register] > 0);
      printf("logic1: %d\n", registers[instr.second_register] > 0 && registers[instr.first_register] < 0);
      printf("\n\n");
      */
      break;
    }
    case je: {
      char ZF = (registers[16] & 0x00000040) >> 6;
      if (ZF)
      {
        program_counter += instr.immediate;
      }
      break;
    }
    case jl: {
      char SF = (registers[16] & 0x00000080) >> 7;
      char OF = (registers[16] & 0x00000800) >> 11;
      if (SF ^ OF)
      {
        program_counter += instr.immediate;
      }
      break;
    }
    case jle: {
      char ZF = (registers[16] & 0x00000040) >> 6;
      char SF = (registers[16] & 0x00000080) >> 7;
      char OF = (registers[16] & 0x00000800) >> 11;
      if ((SF ^ OF) | ZF)
      {
        program_counter += instr.immediate;
      }
      break;
    }
    case jge: {
      char SF = (registers[16] & 0x00000080) >> 7;
      char OF = (registers[16] & 0x00000800) >> 11;
      // printf("~(SF ^ OF): %d\n", ~(SF ^ OF));
      // printf("SF: %d OF: %d\n", SF, OF);
      if (!((SF && !OF) || (!SF && OF)))
      {
        program_counter += instr.immediate;
      }
      break;
    }
    case jbe: {
      char CF = (registers[16] & 0x00000001);
      char ZF = (registers[16] & 0x00000040) >> 6;
      /*
      printf("registers[16]: 0x(%x)\n", registers[16]);
      printf("CF | ZF: %d\n", CF | ZF);
      printf("CF: %d ZF: %d\n", CF, ZF);
      */
      if (CF | ZF)
      {
        program_counter += instr.immediate;
      }
      break;
    }
    case jmp:{
      program_counter += instr.immediate;
      break;
    }
    case call: {
      // printf("called sort");
      registers[6] -= 4; // %esp = %esp - 4
      // memory[%esp] = program_counter + 4, return address
      memory[registers[6]] = program_counter + 4; 
      program_counter += instr.immediate; // jump to target
      break;
    }
    case ret: {
      if (registers[6] == 1024) { 
        program_counter = UINT32_MAX;
        return program_counter;
      }
      program_counter = memory[registers[6]]; // program_counter = memory[%esp]
      registers[6] += 4; // %esp = %esp + 4
      return program_counter; // return 
      break;
    }
    case pushl: {
      // printf("hello from push\n");
      registers[6] -= 4;
      memory[registers[6]] = registers[instr.first_register] & 0x000000FF;
      memory[registers[6] + 1] = (registers[instr.first_register] & 0x0000FF00) >> 8;
      memory[registers[6] + 2] = (registers[instr.first_register] & 0x00FF0000) >> 16;
      memory[registers[6] + 3] = (registers[instr.first_register] & 0xFF000000) >> 24;
      // printf("memory %d %d %d %d: \n", 
      //  memory[registers[6]], memory[registers[6] - 1], memory[registers[6] - 2], memory[registers[6] - 3]);
      break;
    }
    case popl: {
      // printf("hello from pop\n");
      unsigned int byte1 = memory[registers[6]];
      unsigned int byte2 = (memory[registers[6] + 1]) << 8;
      unsigned int byte3 = (memory[registers[6] + 2]) << 16;
      unsigned int byte4 = (memory[registers[6] + 3]) << 24;
      // printf("byte2 : %d\n", byte2);
      registers[instr.first_register] = byte1 + byte2 + byte3 + byte4;
      registers[6] += 4;
      break;
    }
    case printr:
      printf("%d (0x%x)\n", registers[instr.first_register], registers[instr.first_register]);
      break;
    case readr:
      scanf("%d", &(registers[instr.first_register]));
      break;
  }

  // Do not always return program_counter + 4
  //       Some instructions jump elsewhere
  // program_counter + 4 represents the subsequent instruction
  return program_counter + 4;
}


/*********************************************/
/****  DO NOT MODIFY THE FUNCTIONS BELOW  ****/
/*********************************************/

/*
 * Returns the file size in bytes of the file referred to by the given descriptor
*/
unsigned int get_file_size(int file_descriptor)
{
  struct stat file_stat;
  fstat(file_descriptor, &file_stat);
  return file_stat.st_size;
}

/*
 * Loads the raw bytes of a file into an array of 4-byte units
*/
unsigned int* load_file(int file_descriptor, unsigned int size)
{
  unsigned int* raw_instruction_bytes = (unsigned int*)malloc(size);
  if(raw_instruction_bytes == NULL)
    error_exit("unable to allocate memory for instruction bytes (something went really wrong)");

  int num_read = read(file_descriptor, raw_instruction_bytes, size);

  if(num_read != size)
    error_exit("unable to read file (something went really wrong)");

  return raw_instruction_bytes;
}

/*
 * Prints the opcode, register IDs, and immediate of every instruction, 
 * assuming they have been decoded into the instructions array
*/
void print_instructions(instruction_t* instructions, unsigned int num_instructions)
{
  printf("instructions: \n");
  unsigned int i;
  for(i = 0; i < num_instructions; i++)
  {
    printf("op: %d, reg1: %d, reg2: %d, imm: %d\n", 
	   instructions[i].opcode,
	   instructions[i].first_register,
	   instructions[i].second_register,
	   instructions[i].immediate);
  }
  printf("--------------\n");
}

/*
 * Prints an error and then exits the program with status 1
*/
void error_exit(const char* message)
{
  printf("Error: %s\n", message);
  exit(1);
}
