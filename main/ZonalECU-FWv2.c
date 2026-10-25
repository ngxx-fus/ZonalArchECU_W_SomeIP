#include <stdio.h>
#include "template.h"
#include "subdir/template.h"

void app_main(void)
{
    template_func();
    subdir_template_func();
}


