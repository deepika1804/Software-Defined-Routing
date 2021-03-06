/**
 * @d25_assignment3
 * @author  Deepika Chaudhary <d25@buffalo.edu>
 * @version 1.0
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * @section DESCRIPTION
 *
 * This contains the main function. Add further description here....
 */

/**
 * main function
 *
 * @param  argc Number of arguments
 * @param  argv The argument list
 * @return 0 EXIT_SUCCESS
 */

#include <inttypes.h>
#include "./utils/global.c"
#include "./utils/socket_controller.c"



int main(int argc, char **argv)
{
	/*Start Here*/
	sscanf(argv[1], "%" SCNu16, &CONTROL_PORT);
	printf("\ncontrol port is %u\n",CONTROL_PORT);
	init();

	return 0;
}
