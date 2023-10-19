#include "ascii.h"
#include "../../drivers/screen/terminal.h"
#include "../../drivers/screen/vga.h"

void cowsay(const char *str) {
  terminal_writestring("----------\n");
  terminal_writestring(" <");
  terminal_writestring(str);
  terminal_writestring(">\n");
  terminal_writestring("----------\n");
  terminal_writestring("    ^__^\n");
  terminal_writestring("    (oo)_______\n");
  terminal_writestring("    (__)       )-/\n");
  terminal_writestring("       ||----w |\n");
  terminal_writestring("       ||     ||\n");
}

void shrigma(void) {

  terminal_writestring(
      "                             ::::-.                             \n");
  terminal_writestring(
      "                           -:  -:: -                            \n");
  terminal_writestring(
      "                          :+   ..  .-                           \n");
  terminal_writestring(
      "                        :    *:.     :                          \n");
  terminal_writestring("                        -        ::.  =");
  terminal_setcolour(red);
  terminal_writestring("              S H R I G M A O S v0.01\n");
  terminal_setcolour(white);
  terminal_writestring(
      "                       :.  ....  .::-. :.                       \n");
  terminal_writestring(
      "                     ::    =  =         .:.                     \n");
  terminal_writestring(
      "               ::+  :     .::               ::                  \n");
  terminal_writestring(
      "           :--:. .-  -       ..    ..   :.    |.::              \n");
  terminal_writestring(
      "         .:     ..:.::::::....:....::::::::..       #:          \n");
  terminal_writestring(
      "       :+    ....:.::.-:::.  .-....=    .:.   :.  -  *          \n");
  terminal_writestring(
      "        .=-            .:.             :-:.        :. =         \n");
  terminal_writestring(
      "         :-::.  .::::.-      .         .=:.:...   - -           \n");
  terminal_writestring(
      "           .:::-::.:::.-  __        ...  -.    .:::.-.          \n");
  terminal_writestring(
      "                ......-  =%@*::   ::=**.  =----::::             \n");
  terminal_writestring(
      "                      *   -=-:     :%@%: .:                     \n");
  terminal_writestring(
      "                    .:          .       =                       \n");
  terminal_writestring(
      "                     :      -.   .*     -                       \n");
  terminal_writestring(
      "                    :.                  -                       \n");
  terminal_writestring(
      "                    =    .........:::.   =                      \n");
  terminal_writestring(
      "                  +                       :.                    \n");
  terminal_writestring(
      "                .-                         .:                   \n");
}
