#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

/*
    CUSTOMER ACCOUNT BANKING MANAGEMENT SYSTEM
    Enhancement:
    FREE AI Banking Assistant using Ollama + local Gemma 3 model.

    Requirements:
    1. Windows with curl.exe available.
    2. Install Ollama and download the local Gemma 3 1B model:
       ollama pull gemma3:1b
    3. Ollama must be running in the background.

    No API key, OpenAI account, or paid API is required.
    The selected account details are sent only to the local
    Ollama server running on this computer.
*/

int main_exit;

struct date {
    int month, day, year;
};

struct account {
    char name[60];
    int acc_no, age;
    char address[60];
    char citizenship[15];
    double phone;
    char acc_type[10];
    float amt;
    struct date dob;
    struct date deposit;
};

struct account add, upd, check, rem, transaction;

void menu(void);
void close_program(void);
void view_list(void);
void new_acc(void);
void edit(void);
void transact(void);
void erase(void);
void see(void);
void ai_assistant(void);
void print_ollama_response(const char *filename);

float interest(float t, float amount, int rate) {
    return (rate * t * amount) / 100.0f;
}

void pause_screen(void) {
    printf("\n\nPress Enter to continue...");
    getchar();
    getchar();
}

/* Extracts the first text value from the Responses API JSON.
   This is intentionally simple for a beginner-level project. */

void print_ollama_response(const char *filename) {
    FILE *fp;
    long size;
    char *data;
    char *p;
    char *start;
    char *end;

    fp = fopen(filename, "r");

    if (fp == NULL) {
        printf("\nCould not open AI response file.");
        return;
    }

    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    rewind(fp);

    data = (char *)malloc(size + 1);

    if (data == NULL) {
        fclose(fp);
        printf("\nMemory allocation failed.");
        return;
    }

    fread(data, 1, size, fp);
    data[size] = '\0';
    fclose(fp);

    /*
        Ollama /api/generate with stream:false returns the answer
        in a JSON field named "response".
    */
    p = strstr(data, "\"response\"");

    if (p == NULL) {
        printf("\nAI response could not be read.\n");
        printf("\nRaw response:\n%s\n", data);
        free(data);
        return;
    }

    start = strchr(p + 10, ':');

    if (start == NULL) {
        free(data);
        return;
    }

    start++;

    while (*start == ' ' || *start == '\t' ||
           *start == '\n' || *start == '\r') {
        start++;
    }

    if (*start == '"')
        start++;

    end = start;

    while (*end) {
        if (*end == '"' && *(end - 1) != '\\')
            break;
        end++;
    }

    printf("\n---------------------------------------------\n");
    printf("LOCAL AI BANKING ASSISTANT RESPONSE\n");
    printf("---------------------------------------------\n");

    while (start < end) {
        if (*start == '\\' && *(start + 1) == 'n') {
            putchar('\n');
            start += 2;
        } else if (*start == '\\' && *(start + 1) == '"') {
            putchar('"');
            start += 2;
        } else if (*start == '\\' && *(start + 1) == '\\') {
            putchar('\\');
            start += 2;
        } else {
            putchar(*start);
            start++;
        }
    }

    printf("\n---------------------------------------------\n");

    free(data);
}


void ai_assistant(void) {
    int account_no;
    int found = 0;
    char question[300];
    char command[700];
    FILE *fp;

    system("cls");
    printf("\n=============================================\n");
    printf("          FREE AI BANKING ASSISTANT\n");
    printf("=============================================\n");

    printf("\nThis version uses Ollama locally.");
    printf("\nNo OpenAI API key or payment is required.\n");

    printf("\nEnter customer account number: ");
    scanf("%d", &account_no);
    getchar();

    fp = fopen("record.dat", "r");

    if (fp == NULL) {
        printf("\nNo customer records found.");
        pause_screen();
        menu();
        return;
    }

    while (fscanf(fp, "%d %59s %d/%d/%d %d %59s %14s %lf %9s %f %d/%d/%d",
                  &add.acc_no,
                  add.name,
                  &add.dob.month, &add.dob.day, &add.dob.year,
                  &add.age,
                  add.address,
                  add.citizenship,
                  &add.phone,
                  add.acc_type,
                  &add.amt,
                  &add.deposit.month, &add.deposit.day, &add.deposit.year) != EOF) {

        if (add.acc_no == account_no) {
            found = 1;
            break;
        }
    }

    fclose(fp);

    if (!found) {
        printf("\nAccount not found.");
        pause_screen();
        menu();
        return;
    }

    printf("\nAccount found for: %s\n", add.name);
    printf("\nAsk the AI a question about this account.");
    printf("\nExample: Explain my balance and interest in simple language.");
    printf("\nQuestion: ");

    fgets(question, sizeof(question), stdin);
    question[strcspn(question, "\n")] = '\0';

    /*
        PROMPT ENGINEERING USED:
        1. Role prompting - defines the AI as a banking assistant.
        2. Context prompting - supplies selected account information.
        3. Few-shot prompting - gives examples of expected answers.
        4. Constraint prompting - tells the AI not to invent data
           or perform transactions.
    */

    fp = fopen("ai_request.json", "w");

    if (fp == NULL) {
        printf("\nUnable to create AI request file.");
        pause_screen();
        menu();
        return;
    }

    fprintf(fp, "{");
    fprintf(fp, "\"model\":\"gemma3:1b\",");
    fprintf(fp, "\"prompt\":\"");

    write_json_escaped(fp,
        "You are a simple banking information assistant for a beginner "
        "C programming project. Answer clearly and briefly using only "
        "the account information provided below. Never invent account "
        "details, transactions, policies, or interest rates. Never ask "
        "the program to modify, delete, deposit, or withdraw money. "
        "If the answer is not available from the provided information, "
        "say that it is not available. "
        "Example 1: If the user asks for the balance, state the current balance. "
        "Example 2: If the user asks about account type, explain the account type. "
        "Now answer the customer's question.\n\nACCOUNT INFORMATION:\n");

    fprintf(fp, "Account Number: %d\\n", add.acc_no);

    fprintf(fp, "Name: ");
    write_json_escaped(fp, add.name);

    fprintf(fp, "\\nAccount Type: ");
    write_json_escaped(fp, add.acc_type);

    fprintf(fp, "\\nCurrent Balance: %.2f\\n", add.amt);

    fprintf(fp, "Deposit Date: %02d/%02d/%04d\\n",
            add.deposit.month, add.deposit.day, add.deposit.year);

    fprintf(fp, "Customer Question: ");
    write_json_escaped(fp, question);

    fprintf(fp, "\",\"stream\":false");
    fprintf(fp, "}");

    fclose(fp);

    printf("\nContacting local AI model...\n");

    /*
        Ollama runs a local API at:
        http://localhost:11434/api/generate

        No authentication is required for local API access.
    */
    sprintf(command,
        "curl.exe -s http://localhost:11434/api/generate "
        "-H \"Content-Type: application/json\" "
        "--data-binary \"@ai_request.json\" "
        "-o ai_response.json");

    if (system(command) != 0) {
        printf("\nCould not contact Ollama.");
        printf("\nMake sure Ollama is installed and running.");
        pause_screen();
        menu();
        return;
    }

    print_ollama_response("ai_response.json");

    remove("ai_request.json");
    remove("ai_response.json");

    pause_screen();
    menu();
}

void new_acc(void) {
    FILE *ptr;
    int duplicate;

    ptr = fopen("record.dat", "a+");
    if (ptr == NULL) {
        printf("\nUnable to open record file.");
        pause_screen();
        menu();
        return;
    }

    system("cls");
    printf("\n=========== CREATE NEW ACCOUNT ===========\n");

    printf("\nEnter today's date (mm/dd/yyyy): ");
    scanf("%d/%d/%d", &add.deposit.month, &add.deposit.day, &add.deposit.year);

    printf("Enter account number: ");
    scanf("%d", &check.acc_no);

    duplicate = 0;
    rewind(ptr);

    while (fscanf(ptr, "%d %59s %d/%d/%d %d %59s %14s %lf %9s %f %d/%d/%d",
                  &add.acc_no, add.name,
                  &add.dob.month, &add.dob.day, &add.dob.year,
                  &add.age, add.address, add.citizenship,
                  &add.phone, add.acc_type, &add.amt,
                  &add.deposit.month, &add.deposit.day, &add.deposit.year) != EOF) {
        if (check.acc_no == add.acc_no) {
            duplicate = 1;
            break;
        }
    }

    if (duplicate) {
        fclose(ptr);
        printf("\nAccount number already exists.");
        pause_screen();
        menu();
        return;
    }

    add.acc_no = check.acc_no;

    printf("Enter name: ");
    scanf("%59s", add.name);

    printf("Enter date of birth (mm/dd/yyyy): ");
    scanf("%d/%d/%d", &add.dob.month, &add.dob.day, &add.dob.year);

    printf("Enter age: ");
    scanf("%d", &add.age);

    printf("Enter address (single word): ");
    scanf("%59s", add.address);

    printf("Enter citizenship number: ");
    scanf("%14s", add.citizenship);

    printf("Enter phone number: ");
    scanf("%lf", &add.phone);

    printf("Enter initial deposit: ");
    scanf("%f", &add.amt);

    printf("\nAccount type:");
    printf("\n1. saving");
    printf("\n2. current");
    printf("\n3. fixed1");
    printf("\n4. fixed2");
    printf("\n5. fixed3");
    printf("\nEnter account type: ");
    scanf("%9s", add.acc_type);

    fprintf(ptr, "%d %s %d/%d/%d %d %s %s %.0lf %s %.2f %d/%d/%d\n",
            add.acc_no, add.name,
            add.dob.month, add.dob.day, add.dob.year,
            add.age, add.address, add.citizenship,
            add.phone, add.acc_type, add.amt,
            add.deposit.month, add.deposit.day, add.deposit.year);

    fclose(ptr);

    printf("\nAccount created successfully!");
    pause_screen();
    menu();
}

void view_list(void) {
    FILE *view;
    int test = 0;

    view = fopen("record.dat", "r");

    if (view == NULL) {
        printf("\nNo records found.");
        pause_screen();
        menu();
        return;
    }

    system("cls");
    printf("\n=========== CUSTOMER LIST ===========\n");
    printf("\nACC NO.\tNAME\t\tADDRESS\t\tPHONE\n");

    while (fscanf(view, "%d %59s %d/%d/%d %d %59s %14s %lf %9s %f %d/%d/%d",
                  &add.acc_no, add.name,
                  &add.dob.month, &add.dob.day, &add.dob.year,
                  &add.age, add.address, add.citizenship,
                  &add.phone, add.acc_type, &add.amt,
                  &add.deposit.month, &add.deposit.day, &add.deposit.year) != EOF) {

        printf("\n%d\t%s\t\t%s\t\t%.0lf",
               add.acc_no, add.name, add.address, add.phone);
        test++;
    }

    fclose(view);

    if (test == 0)
        printf("\n\nNO RECORDS!");

    pause_screen();
    menu();
}

void edit(void) {
    FILE *old, *newrec;
    int choice, test = 0;

    old = fopen("record.dat", "r");
    newrec = fopen("new.dat", "w");

    if (old == NULL || newrec == NULL) {
        printf("\nUnable to open records.");
        if (old) fclose(old);
        if (newrec) fclose(newrec);
        pause_screen();
        menu();
        return;
    }

    system("cls");
    printf("\nEnter account number to update: ");
    scanf("%d", &upd.acc_no);

    while (fscanf(old, "%d %59s %d/%d/%d %d %59s %14s %lf %9s %f %d/%d/%d",
                  &add.acc_no, add.name,
                  &add.dob.month, &add.dob.day, &add.dob.year,
                  &add.age, add.address, add.citizenship,
                  &add.phone, add.acc_type, &add.amt,
                  &add.deposit.month, &add.deposit.day, &add.deposit.year) != EOF) {

        if (add.acc_no == upd.acc_no) {
            test = 1;

            printf("\n1. Update address");
            printf("\n2. Update phone");
            printf("\nChoice: ");
            scanf("%d", &choice);

            if (choice == 1) {
                printf("New address (single word): ");
                scanf("%59s", upd.address);
                strcpy(add.address, upd.address);
            } else if (choice == 2) {
                printf("New phone: ");
                scanf("%lf", &upd.phone);
                add.phone = upd.phone;
            } else {
                printf("\nInvalid choice.");
            }
        }

        fprintf(newrec, "%d %s %d/%d/%d %d %s %s %.0lf %s %.2f %d/%d/%d\n",
                add.acc_no, add.name,
                add.dob.month, add.dob.day, add.dob.year,
                add.age, add.address, add.citizenship,
                add.phone, add.acc_type, add.amt,
                add.deposit.month, add.deposit.day, add.deposit.year);
    }

    fclose(old);
    fclose(newrec);

    remove("record.dat");
    rename("new.dat", "record.dat");

    if (test)
        printf("\nChanges saved!");
    else
        printf("\nRecord not found!");

    pause_screen();
    menu();
}

void transact(void) {
    FILE *old, *newrec;
    int choice, test = 0;

    old = fopen("record.dat", "r");
    newrec = fopen("new.dat", "w");

    if (old == NULL || newrec == NULL) {
        printf("\nUnable to open records.");
        if (old) fclose(old);
        if (newrec) fclose(newrec);
        pause_screen();
        menu();
        return;
    }

    system("cls");
    printf("\nEnter account number: ");
    scanf("%d", &transaction.acc_no);

    while (fscanf(old, "%d %59s %d/%d/%d %d %59s %14s %lf %9s %f %d/%d/%d",
                  &add.acc_no, add.name,
                  &add.dob.month, &add.dob.day, &add.dob.year,
                  &add.age, add.address, add.citizenship,
                  &add.phone, add.acc_type, &add.amt,
                  &add.deposit.month, &add.deposit.day, &add.deposit.year) != EOF) {

        if (add.acc_no == transaction.acc_no) {
            test = 1;

            if (_stricmp(add.acc_type, "fixed1") == 0 ||
                _stricmp(add.acc_type, "fixed2") == 0 ||
                _stricmp(add.acc_type, "fixed3") == 0) {

                printf("\nFixed accounts do not allow normal deposits/withdrawals.");
            } else {
                printf("\n1. Deposit");
                printf("\n2. Withdraw");
                printf("\nChoice: ");
                scanf("%d", &choice);

                printf("Enter amount: ");
                scanf("%f", &transaction.amt);

                if (transaction.amt <= 0) {
                    printf("\nAmount must be greater than zero.");
                } else if (choice == 1) {
                    add.amt += transaction.amt;
                    printf("\nDeposited successfully!");
                } else if (choice == 2) {
                    if (transaction.amt > add.amt) {
                        printf("\nInsufficient balance!");
                    } else {
                        add.amt -= transaction.amt;
                        printf("\nWithdrawn successfully!");
                    }
                } else {
                    printf("\nInvalid transaction choice.");
                }
            }
        }

        fprintf(newrec, "%d %s %d/%d/%d %d %s %s %.0lf %s %.2f %d/%d/%d\n",
                add.acc_no, add.name,
                add.dob.month, add.dob.day, add.dob.year,
                add.age, add.address, add.citizenship,
                add.phone, add.acc_type, add.amt,
                add.deposit.month, add.deposit.day, add.deposit.year);
    }

    fclose(old);
    fclose(newrec);

    remove("record.dat");
    rename("new.dat", "record.dat");

    if (!test)
        printf("\nRecord not found!");

    pause_screen();
    menu();
}

void erase(void) {
    FILE *old, *newrec;
    int test = 0;

    old = fopen("record.dat", "r");
    newrec = fopen("new.dat", "w");

    if (old == NULL || newrec == NULL) {
        printf("\nUnable to open records.");
        if (old) fclose(old);
        if (newrec) fclose(newrec);
        pause_screen();
        menu();
        return;
    }

    system("cls");
    printf("\nEnter account number to delete: ");
    scanf("%d", &rem.acc_no);

    while (fscanf(old, "%d %59s %d/%d/%d %d %59s %14s %lf %9s %f %d/%d/%d",
                  &add.acc_no, add.name,
                  &add.dob.month, &add.dob.day, &add.dob.year,
                  &add.age, add.address, add.citizenship,
                  &add.phone, add.acc_type, &add.amt,
                  &add.deposit.month, &add.deposit.day, &add.deposit.year) != EOF) {

        if (add.acc_no != rem.acc_no) {
            fprintf(newrec, "%d %s %d/%d/%d %d %s %s %.0lf %s %.2f %d/%d/%d\n",
                    add.acc_no, add.name,
                    add.dob.month, add.dob.day, add.dob.year,
                    add.age, add.address, add.citizenship,
                    add.phone, add.acc_type, add.amt,
                    add.deposit.month, add.deposit.day, add.deposit.year);
        } else {
            test = 1;
        }
    }

    fclose(old);
    fclose(newrec);

    remove("record.dat");
    rename("new.dat", "record.dat");

    if (test)
        printf("\nRecord deleted successfully!");
    else
        printf("\nRecord not found!");

    pause_screen();
    menu();
}

void show_interest(void) {
    float time;
    float intrst;
    int rate;

    if (_stricmp(add.acc_type, "fixed1") == 0) {
        time = 1.0f;
        rate = 9;
        intrst = interest(time, add.amt, rate);
        printf("\nEstimated interest: %.2f", intrst);
    } else if (_stricmp(add.acc_type, "fixed2") == 0) {
        time = 2.0f;
        rate = 11;
        intrst = interest(time, add.amt, rate);
        printf("\nEstimated interest: %.2f", intrst);
    } else if (_stricmp(add.acc_type, "fixed3") == 0) {
        time = 3.0f;
        rate = 13;
        intrst = interest(time, add.amt, rate);
        printf("\nEstimated interest: %.2f", intrst);
    } else if (_stricmp(add.acc_type, "saving") == 0) {
        time = 1.0f / 12.0f;
        rate = 8;
        intrst = interest(time, add.amt, rate);
        printf("\nEstimated monthly interest: %.2f", intrst);
    } else if (_stricmp(add.acc_type, "current") == 0) {
        printf("\nCurrent account interest: 0.00");
    }
}

void see(void) {
    FILE *ptr;
    int choice, test = 0;

    ptr = fopen("record.dat", "r");

    if (ptr == NULL) {
        printf("\nNo records found.");
        pause_screen();
        menu();
        return;
    }

    system("cls");
    printf("\n1. Search by account number");
    printf("\n2. Search by name");
    printf("\nChoice: ");
    scanf("%d", &choice);

    if (choice == 1) {
        printf("Enter account number: ");
        scanf("%d", &check.acc_no);

        while (fscanf(ptr, "%d %59s %d/%d/%d %d %59s %14s %lf %9s %f %d/%d/%d",
                      &add.acc_no, add.name,
                      &add.dob.month, &add.dob.day, &add.dob.year,
                      &add.age, add.address, add.citizenship,
                      &add.phone, add.acc_type, &add.amt,
                      &add.deposit.month, &add.deposit.day, &add.deposit.year) != EOF) {

            if (add.acc_no == check.acc_no) {
                test = 1;
                break;
            }
        }
    } else if (choice == 2) {
        printf("Enter name: ");
        scanf("%59s", check.name);

        while (fscanf(ptr, "%d %59s %d/%d/%d %d %59s %14s %lf %9s %f %d/%d/%d",
                      &add.acc_no, add.name,
                      &add.dob.month, &add.dob.day, &add.dob.year,
                      &add.age, add.address, add.citizenship,
                      &add.phone, add.acc_type, &add.amt,
                      &add.deposit.month, &add.deposit.day, &add.deposit.year) != EOF) {

            if (_stricmp(add.name, check.name) == 0) {
                test = 1;
                break;
            }
        }
    }

    fclose(ptr);

    if (test) {
        printf("\n=============================================\n");
        printf("ACCOUNT DETAILS\n");
        printf("=============================================\n");
        printf("Account No. : %d\n", add.acc_no);
        printf("Name        : %s\n", add.name);
        printf("DOB         : %d/%d/%d\n",
               add.dob.month, add.dob.day, add.dob.year);
        printf("Age         : %d\n", add.age);
        printf("Address     : %s\n", add.address);
        printf("Phone       : %.0lf\n", add.phone);
        printf("Account Type: %s\n", add.acc_type);
        printf("Balance     : %.2f\n", add.amt);
        printf("Deposit Date: %d/%d/%d\n",
               add.deposit.month, add.deposit.day, add.deposit.year);

        show_interest();
    } else {
        printf("\nRecord not found!");
    }

    pause_screen();
    menu();
}

void close_program(void) {
    printf("\n\nThank you for using the Customer Account Banking Management System!\n");
}

void menu(void) {
    int choice;

    system("cls");

    printf("\n=============================================\n");
    printf("     CUSTOMER ACCOUNT BANKING SYSTEM\n");
    printf("=============================================\n");

    printf("\n1. Create new account");
    printf("\n2. Update existing account");
    printf("\n3. Deposit / Withdraw");
    printf("\n4. Check account details");
    printf("\n5. Remove account");
    printf("\n6. View customer list");
    printf("\n7. AI Banking Assistant");
    printf("\n8. Exit");

    printf("\n\nEnter your choice: ");
    scanf("%d", &choice);

    switch (choice) {
        case 1: new_acc(); break;
        case 2: edit(); break;
        case 3: transact(); break;
        case 4: see(); break;
        case 5: erase(); break;
        case 6: view_list(); break;
        case 7: ai_assistant(); break;
        case 8: close_program(); break;
        default:
            printf("\nInvalid choice!");
            pause_screen();
            menu();
    }
}

int main(void) {
    char pass[30];
    const char *password = "codewithc";

    system("cls");

    printf("\n=============================================\n");
    printf("       BANKING MANAGEMENT SYSTEM\n");
    printf("=============================================\n");

    printf("\nEnter password: ");
    scanf("%29s", pass);

    if (strcmp(pass, password) == 0) {
        printf("\nPassword Match!\n");
        menu();
    } else {
        printf("\nWrong password!");
    }

    return 0;
}
