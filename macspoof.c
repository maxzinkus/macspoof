#include <unistd.h> 
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <regex.h>
#include <time.h>
#include <stdbool.h>

#define ADDR_BYTES 18
#define BYTE_MAX 256
#define MAX_INPUT 64

const char* version = "macspoof v0.1.7 (alpha) by Max Zinkus <maxzinkus@gmail.com>";

void help_message() {
   printf("-i <id> to specify NIC id of chosen interface, \
         and optionally -m <mac> to specify a colon-delimited \
         (':') mac address to spoof to.\n");
}

char *generate_mac() {
   int i;
   char *new_addr = calloc(1, ADDR_BYTES);
   
   srand(time(NULL)); /* Seed the rng */
   
   for (i = 0; i < ADDR_BYTES-3; i+=3) {
      sprintf(new_addr + i, "%0x", rand() % BYTE_MAX);
      new_addr[i+2] = ':';
   }
   sprintf(new_addr + i, "%0x", rand() % BYTE_MAX);
   return new_addr;
}

bool validate_interface(char *id) {
   regex_t idpat;
   char* idpattern = "^[a-zA-Z0-9]+$";
   char *test_command;

   regcomp(&idpat, idpattern, REG_EXTENDED);

   if (regexec(&idpat, id, 0, NULL, 0) == REG_NOMATCH) {
      return false;
   }
   else {
      test_command = calloc(1, strlen("ifconfig ") + strnlen(id, MAX_INPUT) +
                            strlen(" >/dev/null")+1
                           );
      sprintf(test_command, "%s %s %s", "ifconfig", id, ">/dev/null");
      
      /* checks if it's actually valid
       * returns 0 => returns true; returns 1 => returns false
       */
      return ! (bool) system(test_command); 
   }
}

bool validate_address(char *addr) {
   regex_t addrpat;
   char* addrpattern = "([0-9A-Fa-f]{2}[:]){5}([0-9A-Fa-f]{2})";

   regcomp(&addrpat, addrpattern, REG_EXTENDED);

   if (regexec(&addrpat, addr, 0, NULL, 0) == REG_NOMATCH) {
      return false;
   }
   else {
      return true;
   }
}

void spoof_interface_mac(char* id, char* spoof_addr) {
   char *command;
   command = calloc(1, strlen("ifconfig ") + strnlen(id, MAX_INPUT) + strlen(" ether ") +
                    strnlen(spoof_addr, ADDR_BYTES)+1
                   );
   sprintf(command, "%s %s %s %s", "ifconfig", id, "ether", spoof_addr);
   system(command);
}

void bounce_interface(char *id) {
   char command[13] = "ifconfig ";
   char up_command[16];
   char down_command[18];
   
   strncat(command, id, sizeof(command) - strlen(command)-1);
   strncpy(up_command, command, sizeof(up_command)-1);
   strncpy(down_command, command, sizeof(down_command)-1);
   strncat(up_command, " up", sizeof(up_command) - strlen(up_command)-1);
   strncat(down_command, " down", sizeof(down_command) - strlen(down_command)-1);
   printf("%s\n", down_command);
   system(down_command);
   sleep(1);
   printf("%s\n", up_command);
   system(up_command);
}

int main(int argc, char **argv) {
   char c, *id = NULL, *spoof_addr = NULL;

   opterr = 0;

   while ((c = getopt(argc, argv, "hVi:m:")) != -1) {
      switch(c) {
         case 'h':
            help_message();
            return 0;
         case 'V':
            printf("%s\n", version);
            return 0;
         case 'i':
            if (validate_interface(optarg)) {
               id = calloc(1, strlen(optarg)+1);
               strcpy(id, optarg);
            }
            else {
               fprintf(stderr, "Invalid interface specified after -i.\n");
               return 1;
            }
            break;
         case 'm':
            if (validate_address(optarg)) {
               spoof_addr = calloc(1, strlen(optarg)+1);
               strcpy(spoof_addr, optarg);
            }
            else {
               fprintf(stderr, "Invalid address specified after -m.\n");
               return 1;
            }
            break;
         case '?':
            if (optopt == 'i' || optopt == 'm') {
               fprintf(stderr, "Option -%c requires an argument.\n", optopt);
               return 1;
            }
            else if (isprint(optopt)) {
               fprintf(stderr, "Unknown option -%c.\n", optopt);
               return 1;
            }
            else {
               fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
               return 1;
            }
         default:
            abort(); /* if we get here, bad things have happened */
      }
   }
   if (geteuid() != 0) {
      fprintf(stderr, "This tools needs to be run as root.\n");
      return 1;
   }
   if (!id) { 
      fprintf(stderr, "No interface supplied.\n");
      return 1;
   }
   if (!spoof_addr) { 
      spoof_addr = generate_mac();
      spoof_addr[ADDR_BYTES-1] = '\0';
   }
   spoof_interface_mac(id, spoof_addr);
   sleep(2);
   bounce_interface(id);
   sleep(1);
   return 0;
}
