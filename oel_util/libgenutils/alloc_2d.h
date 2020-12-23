/** @file alloc_2d.h
	@brief Utility functions for allocating and freeing two-dimensional arrays of various types.

	This file was created by alloc_2d.pl and should not be edited manually.
*/
#ifndef OEL_UTIL_LIBGENUTILS_ALLOC_2D_H_
#define OEL_UTIL_LIBGENUTILS_ALLOC_2D_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Allocate a two-dimensional array of type char of a given size.

	The order of the parameters are the reverse of when the data is being accessed.  E.g., this is valid (ignoring the
	printf type specifier):

	char **array = alloc_2d_char(1, 5);
	printf("%d\n", array[4][0]);
	
	@param[in] w Width of array.  NB: this is the second array dimension.
	@param[in] h Height of array.  NB: this is the first array dimension.

	@return A malloc'd array or NULL if any malloc fails.
*/
char **alloc2d_char(size_t w, size_t h);
/** @brief Free a two-dimensional array created by alloc2d_char.

	@param[in] p Pointer to array created by alloc2d_char.
*/
void free2d_char(char **p);
/** @brief Free a two-dimensional array created by malloc'ing each row individually.

	@param[in] p Pointer to array to free.
	@param[in] length Length of input array's first dimension.
*/
void free2d_all_char(char **p, size_t length);

/** @brief Allocate a two-dimensional array of type short of a given size.

	The order of the parameters are the reverse of when the data is being accessed.  E.g., this is valid (ignoring the
	printf type specifier):

	short **array = alloc_2d_short(1, 5);
	printf("%d\n", array[4][0]);
	
	@param[in] w Width of array.  NB: this is the second array dimension.
	@param[in] h Height of array.  NB: this is the first array dimension.

	@return A malloc'd array or NULL if any malloc fails.
*/
short **alloc2d_short(size_t w, size_t h);
/** @brief Free a two-dimensional array created by alloc2d_short.

	@param[in] p Pointer to array created by alloc2d_short.
*/
void free2d_short(short **p);
/** @brief Free a two-dimensional array created by malloc'ing each row individually.

	@param[in] p Pointer to array to free.
	@param[in] length Length of input array's first dimension.
*/
void free2d_all_short(short **p, size_t length);

/** @brief Allocate a two-dimensional array of type float of a given size.

	The order of the parameters are the reverse of when the data is being accessed.  E.g., this is valid (ignoring the
	printf type specifier):

	float **array = alloc_2d_float(1, 5);
	printf("%d\n", array[4][0]);
	
	@param[in] w Width of array.  NB: this is the second array dimension.
	@param[in] h Height of array.  NB: this is the first array dimension.

	@return A malloc'd array or NULL if any malloc fails.
*/
float **alloc2d_float(size_t w, size_t h);
/** @brief Free a two-dimensional array created by alloc2d_float.

	@param[in] p Pointer to array created by alloc2d_float.
*/
void free2d_float(float **p);
/** @brief Free a two-dimensional array created by malloc'ing each row individually.

	@param[in] p Pointer to array to free.
	@param[in] length Length of input array's first dimension.
*/
void free2d_all_float(float **p, size_t length);

/** @brief Allocate a two-dimensional array of type double of a given size.

	The order of the parameters are the reverse of when the data is being accessed.  E.g., this is valid (ignoring the
	printf type specifier):

	double **array = alloc_2d_double(1, 5);
	printf("%d\n", array[4][0]);
	
	@param[in] w Width of array.  NB: this is the second array dimension.
	@param[in] h Height of array.  NB: this is the first array dimension.

	@return A malloc'd array or NULL if any malloc fails.
*/
double **alloc2d_double(size_t w, size_t h);
/** @brief Free a two-dimensional array created by alloc2d_double.

	@param[in] p Pointer to array created by alloc2d_double.
*/
void free2d_double(double **p);
/** @brief Free a two-dimensional array created by malloc'ing each row individually.

	@param[in] p Pointer to array to free.
	@param[in] length Length of input array's first dimension.
*/
void free2d_all_double(double **p, size_t length);

#ifdef __cplusplus
}
#endif

#endif /* OEL_UTIL_LIBGENUTILS_ALLOC_2D_H_ */
