#include "asciifolding.h"

#include <string.h>
#include <stdio.h>

int main() {
    unsigned char input[] = "Des mot clés À LA CHAÎNE À Á Â Ã Ä Å Æ Ç È É Ê Ë Ì Í Î Ï Ĳ Ð Ñ Ò Ó Ô Õ Ö Ø Œ Þ Ù Ú Û Ü Ý Ÿ à á â ã ä å æ ç è é ê ë ì í î ï ĳ ð ñ ò ó ô õ ö ø œ ß þ ù ú û ü ý ÿ ﬁ ﬂ";

    unsigned int input_length = strlen(input);

    unsigned char output[1024] = { 0 };

    unsigned int output_length = asciifolding(input, input_length, output);
    printf("output_length : %d\n", output_length);
    printf("output : %s\n", output);

    return 0;
}
