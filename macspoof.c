#include<unistd.h> // geteuid, getopt, sleep
#include<stdio.h> // stderr, printf
#include<stdlib.h> // srand, rand, fprintf, strncpy, strchr, system, exit
#include<string.h> // strncat, strlen
#include<ctype.h> // isprint
#include<regex.h> // regex (...duh)
#include<time.h> // time (...also duh)
#include<stdbool.h> // cuz C doesn't have bools?!

char* version = "macspoof v1.5 (alpha) by Max Zinkus <maxzinkus@gmail.com>";

int help_message()
{
    fprintf(stderr, "No full help message defined.\n");
    printf("-i <id> to specify NIC id of chosen interface, and optionally -m <mac> to specify a colon-delimited (':') mac address to spoof to.\n");
    return 0;
}

void generate_mac(char* new_addr) // pass in memory so that we aren't returning a local stack variable
{
    srand(time(NULL)); // Seed the rng
    char hexbytes[17] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'}; // Valid hex digits
    for (int i = 0; i<17; i++)
    {
        if ((i-2) % 3 != 0) // 2, 5, 8, 11, 14 are where the ':' go
        {
            new_addr[i] = hexbytes[rand() % 16];
        }
        else
        {
            new_addr[i] = ':';
        }
    }
}

bool validate_interface(char *id)
{
    if (strlen(id) != 3)
    {
        return false;
    }
    regex_t idpat;
    int reti;
    char* idpattern = "^[a-zA-Z0-9]{3}$"; // should be three alphanumeric characters

    reti = regcomp(&idpat, idpattern, REG_EXTENDED);

    if (reti)
    {
        fprintf(stderr, "Could not compile regex.\n");
        exit(1);
    }

    reti = regexec(&idpat, id, 0, NULL, 0);

    if (reti == REG_NOMATCH)
    {
        return false;
    }
    else if (!reti)
    {
        char test_command[32] = "ifconfig ";
        strncat(test_command, id, sizeof(test_command)-1 - strlen(test_command));
        strncat(test_command, " >/dev/null", sizeof(test_command)-1 - strlen(test_command));
        return ! (bool) system(test_command); // checks if it's actually valid. returns 0 => returns true; returns 1 => returns false
    }
    else
    {
        fprintf(stderr, "Regex matching error.\n");
        exit(1);
    }
}

bool validate_address(char *addr)
{
    if (strlen(addr) != 17)
    {
        return false;
    }
    regex_t addrpat;
    int reti;
    char* addrpattern = "([0-9A-Fa-f]{2}[:]){5}([0-9A-Fa-f]{2})"; // should match two hex digits followed by a colon five times, and then two more hex digits
    
    reti = regcomp(&addrpat, addrpattern, REG_EXTENDED);
    
    if (reti)
    {
        fprintf(stderr, "Could not compile regex.\n");
        exit(1);
    }
    
    reti = regexec(&addrpat, addr, 0, NULL, 0);
    
    if (!reti)
    {
        return true;
    } 
    else if (reti == REG_NOMATCH)
    {
        return false;
    }
    else
    {
        fprintf(stderr, "Regex matching error.\n");
        exit(1);
    }
}

int main(int argc, char **argv)
{
    char id[4];
    char spoof_addr[18];

    opterr = 0;

    char c;
    
    while ((c = getopt(argc, argv, "hVi:m:")) != -1) // standard getopt procedure
    {
        switch(c)
        {
            case 104: // h
                exit(help_message());
                break;
            case 86: // V
                printf("%s\n", version);
                return 0;
            case 105: // i
                if (validate_interface(optarg))
                {
                    strncpy(id, optarg, sizeof(id)-1);
                    id[sizeof(id)-1] = '\0';
                }
                else
                {
                    fprintf(stderr, "Invalid interface specified after -i.\n");
                    return 1;
                }
                break;
            case 109: // m
                if (validate_address(optarg))
                {
                    strncpy(spoof_addr, optarg, sizeof(spoof_addr)-1);
                    spoof_addr[sizeof(spoof_addr)-1] = '\0';
                }
                else
                {
                    fprintf(stderr, "Invalid address specified after -m.\n");
                    return 1;
                }
                break;
            case 63: // ?
                if (optopt == 'i' || optopt == 'm')
                {
                    fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                    return 1;
                }
                else if (isprint(optopt))
                {
                    fprintf(stderr, "Unknown option -%c.\n", optopt);
                    return 1;
                }
                else
                {
                    fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
                    return 1;
                }
            default:
                abort(); // if we get here, bad things have happened
        }
    }
    if (geteuid() != 0) // we need teh rootz
    {
        fprintf(stderr, "This tools needs to be run as root.\n");
        return 1;
    }
    if (strlen(id) != 3) // validate_id rejects anything not 3 long
    {
        fprintf(stderr, "No interface supplied.\n");
        return 1;
    }
    if (strlen(spoof_addr) != 17) // validate_address rejects anything not 17 long
    {
        char new_addr[18];
        generate_mac(new_addr);
        new_addr[sizeof(new_addr)-1] = '\0';
        strncpy(spoof_addr, new_addr, sizeof(spoof_addr)-1);
        spoof_addr[sizeof(spoof_addr)-1] = '\0';
    }
    char command[37] = "ifconfig ";
    strncat(command, id, sizeof(command)-1 - strlen(command));
    strncat(command, " ether ", sizeof(command)-1 - strlen(command));
    strncat(command, spoof_addr, sizeof(command)-1 - strlen(command));
    system(command);
    sleep(2);
    strncpy(command, "ifconfig ", sizeof(command)-1);
    strncat(command, id, sizeof(command)-1 - strlen(command));
    strncat(command, " down", sizeof(command)-1 - strlen(command));
    system(command);
    sleep(1);
    strncpy(command, "ifconfig ", sizeof(command)-1);
    strncat(command, id, sizeof(command)-1 - strlen(command));
    strncat(command, " up", sizeof(command)-1 - strlen(command));
    system(command);
    return 0;
}
