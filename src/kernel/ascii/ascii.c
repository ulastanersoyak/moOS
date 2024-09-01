#include "ascii.h"
#include "../../drivers/screen/vga.h"
#include "../../libc/stdio/stdio.h"
#include "../../libc/string/string.h"

void
cowsay (const char *str, enum vga_colour colour)
{
  size_t size = strlen (str);
  printf ("  ");
  for (size_t i = 0; i < size; i++)
    {
      printf ("=");
    }
  printf ("\n<%C %s %C>\n", colour, str, white);
  printf ("  ");
  for (size_t i = 0; i < size; i++)
    {
      printf ("=");
    }
  printf ("\n    ^__^\n");
  printf ("    (oo)_______\n");
  printf ("    (__)       )-/\n");
  printf ("       ||----w |\n");
  printf ("       ||     ||\n");
}

void
shrigma (void)
{
  printf ("                             \n");
  printf ("                           -:  -:: -\n");
  printf ("                          :+   ..  .-\n");
  printf ("                        :    *:.     :\n");
  printf ("                        -        ::.  =\n");
  printf ("                       :.  ....  .::-. :\n");
  printf ("                     ::    =  =         .:.\n");
  printf ("               ::+  :     .::               ::\n");
  printf ("           :--:. .-  -       ..    ..   :.    |.::\n");
  printf ("         .:     ..:.::::::....:....::::::::..       #:\n");
  printf ("       :+    ....:.::.-:::.  .-....=    .:.   :.  -  *\n");
  printf ("        .=-            .:.             :-:.        :. =\n");
  printf ("         :-::.  .::::.-      .         .=:.:...   - -\n");
  printf ("           .:::-::.:::.-  __        ...  -.    .:::.-.\n");
  printf ("                ......-  =%@*::   ::=**.  =----::::\n");
  printf ("                      *   -=-:     :%@%: .:\n");
  printf ("                    .:          .       =\n");
  printf ("                     :      -.   .*     -\n");
  printf ("                    :.                  -\n");
  printf ("                    =    .........:::.   =\n");
  printf ("                  +                       :.\n");
  printf ("                .-                         .:\n");
}

void
moose (const char *str, enum vga_colour colour)
{
  printf ("                                  ___            ___\n");
  printf ("                                 /   \\          /   \\\n");
  printf ("                                 \\_   \\        /  __/\n");
  printf ("                                  _\\   \\      /  /__\n");
  printf ("                                  \\___  \\____/   __/\n");
  printf ("                                      \\_       _/\n");
  printf ("%C %s %C", colour, str, white);
  int32_t space = 36 - strlen (str);
  space = (space < 0) ? 0 : space;
  for (size_t i = 0; i < space; i++)
    {
      printf (" ");
    }
  printf ("| @ @  \\_\n");
  printf ("                                        |\n");
  printf ("                                      _/     /\\\n");
  printf ("                                     /o)  (o/\\ \\_\n");
  printf ("                                     \\_____/ /\n");
  printf ("                                       \\____/\n");
}
