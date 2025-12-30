#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <math.h>
#include <windows.h>

// --- Constants & Structures ---
#define MAX_VARS 500
#define MAX_FUNCS 100
#define MAX_STR 256

typedef enum { TYPE_NUM, TYPE_STR, TYPE_BOOL } VarType;

typedef struct {
    char name[32];
    double num;
    char str[MAX_STR];
    VarType type;
    int scope_level; 
} Variable;

typedef struct {
    char name[32];
    char arg_name[32];
    char body[2048];
} Function;

Variable memory[MAX_VARS];
int var_count = 0;
Function funcs[MAX_FUNCS];
int func_count = 0;
int current_scope = 0;

// --- Prototypes ---
void execute_line(char* line);
void execute_block(char* block);
double get_val(char* token);

// --- Core Helper Functions ---
void set_var(char* name, double n, char* s, VarType t) {
    for (int i = 0; i < var_count; i++) {
        if (strcmp(memory[i].name, name) == 0 && memory[i].scope_level == current_scope) {
            memory[i].num = n; 
            if(s) strcpy(memory[i].str, s); 
            memory[i].type = t; 
            return;
        }
    }
    if (var_count < MAX_VARS) {
        strcpy(memory[var_count].name, name);
        memory[var_count].num = n; 
        if(s) strcpy(memory[var_count].str, s);
        memory[var_count].type = t; 
        memory[var_count].scope_level = current_scope;
        var_count++;
    }
}

void clear_local_vars() {
    int i = 0;
    while (i < var_count) {
        if (memory[i].scope_level > 0) {
            for (int j = i; j < var_count - 1; j++) memory[j] = memory[j+1];
            var_count--;
        } else i++;
    }
}

Variable* find_var(char* name) {
    for (int i = var_count - 1; i >= 0; i--) {
        if (strcmp(memory[i].name, name) == 0) return &memory[i];
    }
    return NULL;
}

// --- 비교 함수 (람다 대신 일반 함수로 변경) ---
double compare_values(double v1, char* o, double v2) {
    if (strcmp(o, ">") == 0) return v1 > v2;
    if (strcmp(o, "<") == 0) return v1 < v2;
    if (strcmp(o, "==") == 0) return v1 == v2;
    if (strcmp(o, "!=") == 0) return v1 != v2;
    if (strcmp(o, ">=") == 0) return v1 >= v2;
    if (strcmp(o, "<=") == 0) return v1 <= v2;
    return 0;
}

// --- Expression Evaluator ---
double eval_logic(char* cond) {
    char l[64], op[4], r[64], logic[8], l2[64], op2[4], r2[64];
    int n = sscanf(cond, "%s %s %s %s %s %s %s", l, op, r, logic, l2, op2, r2);
    
    double res1 = compare_values(get_val(l), op, get_val(r));
    if (n > 3) {
        double res2 = compare_values(get_val(l2), op2, get_val(r2));
        if (strcmp(logic, "and") == 0) return (res1 && res2);
        if (strcmp(logic, "or") == 0) return (res1 || res2);
    }
    return res1;
}

// --- Built-in Math ---
double run_builtin_math(char* line) {
    char a1[64], a2[64];
    if (sscanf(line, "sqrt %s", a1) == 1) return sqrt(get_val(a1));
    if (sscanf(line, "pow %s %s", a1, a2) == 2) return pow(get_val(a1), get_val(a2));
    if (sscanf(line, "abs %s", a1) == 1) return fabs(get_val(a1));
    if (strcmp(line, "time") == 0) return (double)time(NULL);
    if (sscanf(line, "len %s", a1) == 1) {
        Variable* v = find_var(a1);
        return v ? (double)strlen(v->str) : 0;
    }
    return get_val(line);
}

// --- Engine ---
void execute_block(char* block) {
    char temp[2048];
    strcpy(temp, block);
    char* cmd = strtok(temp, ";");
    while (cmd) {
        execute_line(cmd);
        cmd = strtok(NULL, ";");
    }
}

void execute_line(char* line) {
    while (*line == ' ' || *line == '\t') line++;
    if (!*line || *line == '#') return;

    // FOR loop
    if (strncmp(line, "for ", 4) == 0) {
        char var[32], then_part[1024]; int start, end;
        if (sscanf(line, "for %s from %d to %d then %[^\n]", var, &start, &end, then_part) == 4) {
            for (int i = start; i <= end; i++) {
                set_var(var, (double)i, NULL, TYPE_NUM);
                execute_block(then_part);
            }
        }
        return;
    }

    // WHILE loop
    if (strncmp(line, "while ", 6) == 0) {
        char cond[128], then_part[1024];
        char* tp = strstr(line, " then ");
        if (tp) {
            strncpy(cond, line+6, tp-(line+6)); cond[tp-(line+6)] = '\0';
            while (eval_logic(cond)) execute_block(tp + 6);
        }
        return;
    }

    // IF-OTHERWISE
    if (strncmp(line, "if ", 3) == 0) {
        char cond[128]; 
        char* tp = strstr(line, " then ");
        char* ep = strstr(line, " otherwise ");
        if (tp) {
            strncpy(cond, line+3, tp-(line+3)); cond[tp-(line+3)] = '\0';
            if (eval_logic(cond)) {
                if (ep) { 
                    char act[1024] = {0}; 
                    strncpy(act, tp+6, ep-(tp+6)); 
                    execute_block(act); 
                }
                else execute_block(tp+6);
            } else if (ep) execute_block(ep+11);
        }
        return;
    }

    // FUNCTION definition
    if (strncmp(line, "function ", 9) == 0) {
        char n[32], a[32]; 
        char* s = strstr(line, " start "); 
        char* e = strrchr(line, ' ');
        if (s) {
            sscanf(line, "function %s %s", n, a);
            strcpy(funcs[func_count].name, n); 
            strcpy(funcs[func_count].arg_name, a);
            strcpy(funcs[func_count++].body, s+7);
            printf("Function '%s' drawing complete.\n", n);
        }
        return;
    }

    // CALL function
    if (strncmp(line, "call ", 5) == 0) {
        char n[32], v[64]; sscanf(line, "call %s %s", n, v);
        for (int i=0; i<func_count; i++) {
            if (strcmp(funcs[i].name, n) == 0) {
                current_scope++;
                set_var(funcs[i].arg_name, get_val(v), NULL, TYPE_NUM);
                execute_block(funcs[i].body);
                clear_local_vars();
                current_scope--;
                return;
            }
        }
        return;
    }

    // SET variable
    if (strncmp(line, "set ", 4) == 0) {
        char name[32], rhs[256];
        if (sscanf(line, "set %s = %[^\n]", name, rhs) == 2) {
            if (rhs[0] == '"') {
                char s[MAX_STR] = {0}; sscanf(rhs, "\"%[^\"]\"", s); 
                set_var(name, 0, s, TYPE_STR);
            } else {
                set_var(name, run_builtin_math(rhs), NULL, TYPE_NUM);
            }
        }
        return;
    }

    // SAY command
    if (strncmp(line, "say ", 4) == 0) {
        char* t = line+4;
        if (*t == '"') { 
            char s[MAX_STR] = {0}; sscanf(t, "\"%[^\"]\"", s); 
            printf("%s\n", s); 
        } else {
            Variable* v = find_var(t);
            if (v) {
                if (v->type == TYPE_STR) printf("%s\n", v->str);
                else printf("%.2f\n", v->num);
            } else printf("%.2f\n", run_builtin_math(t));
        }
    }
    
    // Commands: cls, list
    if (strcmp(line, "cls") == 0) system("cls");
    if (strcmp(line, "list") == 0) {
        for(int i=0; i<var_count; i++) 
            printf("[%d] %s = %.2f / %s\n", memory[i].scope_level, memory[i].name, memory[i].num, memory[i].str);
    }
}

double get_val(char* t) {
    while (*t == ' ') t++;
    if (isdigit(*t) || (*t=='-' && isdigit(*(t+1)))) return atof(t);
    Variable* v = find_var(t);
    return v ? v->num : 0;
}

int main() {
    char b[1024]; srand((unsigned int)time(NULL));
    printf("--- Pencil Super-Update (Pure C Fix) ---\n");
    while (1) {
        printf("pencil> "); 
        if (!fgets(b, 1024, stdin)) break;
        b[strcspn(b, "\n\r")] = 0;
        if (strcmp(b, "exit") == 0) break;
        execute_line(b);
    }
    return 0;
}