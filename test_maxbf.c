/**
 * Test MaxBF. Some test cases taken from http://brainfuck.org/, written by
 * Daniel B Cristofani (cristofdathevanetdotcom) and shared under the Creative
 * Commons Attribution-ShareAlike 4.0 International License.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "maxbf.c"
#include "minunit.h"

int tests_run = 0;
int tmpnam_calls = 0;

/*** Setup functions ***/
FILE *create_file_from_string(const char *s)
{
    char file_name[L_tmpnam];
    if (tmpnam_calls == TMP_MAX || tmpnam(file_name) == NULL) {
        goto error;
    }
    tmpnam_calls++;

    FILE *fp = fopen(file_name, "w+");
    if (fp == NULL) {
        goto error;
    }

    if (fputs(s, fp) == EOF) {
        goto error;
    }
    fseek(fp, 0L, SEEK_SET);

    return fp;

error:
    puts("Could not generate temporary file for tests.");
    exit(EXIT_FAILURE);
}

void buf_cleanup(void)
{
    mock_input_buf[0] = '\0';
    mock_output_buf[0] = '\0';
}

/*** Test cases ***/
static char *test_hello_world()
{
    FILE *fp = create_file_from_string(">++++++++[<+++++++++>-]<.>++++[<+++++++>-]<+.+++++++..+++.>>++++++[<+++++++>-]<+"
                                       "+.------------.>++++++[<+++++++++>-]<+.<.+++.------.--------.>>>++++[<++++++++>-"
                                       "]<+.");

    ExcecutionStatus status = excecute_brainfuck_from_stream(fp);
    mu_assert("Error, Hello World failed.", strcmp(mock_output_buf, "Hello, World!") == 0 && status == STATUS_OK);

    fclose(fp);
    buf_cleanup();
    return 0;
}

static char *test_array_size()
{
    FILE *fp = create_file_from_string("++++[>++++++<-]>[>+++++>+++++++<<-]>>++++<[[>[[>>+<<-]<]>>>-]>-[>+>+<<-]>]"
                                       "+++++[>+++++++<<++>-]>.<<.");

    ExcecutionStatus status = excecute_brainfuck_from_stream(fp);
    mu_assert("Error, Check for cell 30,000 failed.", strcmp(mock_output_buf, "#\n") == 0 && status == STATUS_OK);

    fclose(fp);
    buf_cleanup();
    return 0;
}

static char *test_left_bound()
{
    FILE *fp = create_file_from_string("<");

    ExcecutionStatus status = excecute_brainfuck_from_stream(fp);
    mu_assert("Error, Moving left from cell 0 did not result in an error.", status == STATUS_ERR_LBOUND);

    fclose(fp);
    buf_cleanup();
    return 0;
}

static char *test_improper_nesting_1()
{
    FILE *fp = create_file_from_string("[[][][[]]");

    ExcecutionStatus status = excecute_brainfuck_from_stream(fp);
    mu_assert("Error, Improper nesting (extra [) did not cause an error.", status == STATUS_ERR_NESTING);

    fclose(fp);
    buf_cleanup();
    return 0;
}

static char *test_improper_nesting_2()
{
    FILE *fp = create_file_from_string("[[]][[]");

    ExcecutionStatus status = excecute_brainfuck_from_stream(fp);
    mu_assert("Error, Improper nesting (extra ]) did not cause an error.", status == STATUS_ERR_NESTING);

    fclose(fp);
    buf_cleanup();
    return 0;
}

static char *test_ignore_characters()
{
    FILE *fp = create_file_from_string("abcd[efg]123?");

    ExcecutionStatus status = excecute_brainfuck_from_stream(fp);
    mu_assert("Error, Invalid characters were not ignored.", status == STATUS_OK);

    fclose(fp);
    buf_cleanup();
    return 0;
}

static char *test_bracket_skipping()
{
    FILE *fp = create_file_from_string("[This: < and this [<] shouldn't cause an error]");

    ExcecutionStatus status = excecute_brainfuck_from_stream(fp);
    mu_assert("Error, Valid nesting caused an error.", status == STATUS_OK);

    fclose(fp);
    buf_cleanup();
    return 0;
}

static char *test_obscure_problems()
{
    FILE *fp = create_file_from_string("[]++++++++++[>>+>+>++++++[<<+<+++>>>-]<<<<-]"
                                       "\"A*$\";?@![#>>+<<]>[>>]<<<<[>++<[-]]>.>.");

    ExcecutionStatus status = excecute_brainfuck_from_stream(fp);
    mu_assert("Error, Check for several obscure errors failed.", strcmp(mock_output_buf, "H\n") == 0 && status == STATUS_OK);

    fclose(fp);
    buf_cleanup();
    return 0;
}

static char *test_input()
{
    FILE *fp = create_file_from_string(",.,.,,.>,.");
    strcpy(mock_input_buf, "Y\n&?.");

    ExcecutionStatus status = excecute_brainfuck_from_stream(fp);
    mu_assert("Error, Input check failed.", strcmp(mock_output_buf, "Y\n?.") == 0 && status == STATUS_OK);

    fclose(fp);
    buf_cleanup();
    return 0;
}

static char *test_io()
{
    FILE *fp = create_file_from_string(">,>+++++++++,>+++++++++++[<++++++<++++++<+>>>-]<<.>.<<-.>.>.<<.");
    // Newline + EOF
    strcpy(mock_input_buf, "\n");

    ExcecutionStatus status = excecute_brainfuck_from_stream(fp);
    mu_assert("Error, Input check failed.", strcmp(mock_output_buf, "LB\nLB\n") == 0 && status == STATUS_OK);

    fclose(fp);
    buf_cleanup();
    return 0;
}

static char *all_tests()
{
    mu_run_test(test_hello_world);
    mu_run_test(test_array_size);
    mu_run_test(test_left_bound);
    mu_run_test(test_improper_nesting_1);
    mu_run_test(test_improper_nesting_2);
    mu_run_test(test_ignore_characters);
    mu_run_test(test_bracket_skipping);
    mu_run_test(test_obscure_problems);
    mu_run_test(test_input);
    mu_run_test(test_io);

    return 0;
}

int main(void)
{
    char *result = all_tests();
    if (result != 0) {
        printf("%s\n", result);
    } else {
        printf("ALL TESTS PASSED\n");
    }
    printf("Tests run: %d\n", tests_run);

    return result != 0;
}
