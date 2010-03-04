/* on 64-bit systems, some "black magic" is needed for many castings,
 * as sizeof(void *) is bigger than sizeof(int *) -- as the code,
 * written for 32-bits, was expecting. Most of the time, this is
 * solved by first casting into long int.
 * While it is possible in theory to loose the first bits with this
 * casting, in practice it does not happen as the values are always
 * integers and never long integers -- in fact, we are only using
 * long int for casting purposes. That being said, the use of #defines
 * and #ifdef is considered a bad practice; if you know enough C
 * to make this elegant, please fix (or even better, teach me!). - TT
 */

/*#define SYS32BIT */
#define SYS64BIT
