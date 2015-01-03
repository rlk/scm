#include <stdio.h>
#include <fcntl.h>

#ifdef _WIN32
#include <io.h>
#endif

int main(int argc, char **argv)
{
    int n = 0;
    int c;

    if (argc > 1)
        printf("unsigned char %s[] = {\n", argv[1]);

#ifdef _WIN32
    _setmode(_fileno(stdin), _O_BINARY);
#endif

    do
    {
        printf("\t");

        while ((c = getchar()) != EOF)
        {
            printf("0x%02x, ", c);
            if (++n % 12 == 0)
                break;
        }

        printf("\n");
    }
    while (c != EOF);

    if (argc > 1)
        printf("};\nunsigned int %s_len = %d;\n", argv[1], n);

    return 0;
}
