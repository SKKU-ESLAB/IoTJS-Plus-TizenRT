/****************************************************************************
 *
 * Copyright 2019 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>

extern uint32_t __kflash_segment_start__[];

/****************************************************************************
 *
 * Name: write_code_main
 *
 * Description: Test Case Example for write to kernel code space from user space
 *
 ****************************************************************************/

int write_code_main(void)
{
	uint32_t *address = (uint32_t *)(__kflash_segment_start__);
	uint32_t dest = 0xdeadbeef;

	printf("\n************************************************\n");
	printf("* Test to verify protection of Kernel code     *\n");
	printf("* User Tasks should not be allowed to write    *\n");
	printf("* kernel code space. MPU shall raise exception *\n");
	printf("************************************************\n");

	sleep(1);
	*address = dest;

	printf("INFO: User Task successfully write to Kernel code space\n");
	return 0;
}