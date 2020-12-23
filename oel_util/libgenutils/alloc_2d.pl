#!/usr/bin/env perl

use strict;
use warnings FATAL => 'all';

use FindBin;
use Data::Dumper;

my @types = (
	['char', '_char'],
	#['unsigned char', '_uchar'],
	#['signed char', '_schar'],
	['short', '_short'],
	#['int', '_int'],
	#['long', '_long'],
	['float', '_float'],
	['double', '_double'],
	#['unsigned short', '_ushort'],
	#['unsigned int', '_uint'],
	#['long long', '_longlong'],
	#['unsigned long long', '_ulonglong'],
);

sub generate_alloc2d {
	#my @args = @_;
	
	my %templates = read_templates();

	my $c_file = "$FindBin::RealDir/alloc_2d.c";
	my $h_file = "$FindBin::RealDir/alloc_2d.h";

	open(my $c_fh, '>', $c_file) || die("Couldn't open $c_file: $!");
	open(my $h_fh, '>', $h_file) || die("Couldn't open $h_file: $!");
	print {$c_fh} $templates{c_header};
	print {$h_fh} $templates{h_header};
	for (@types){
		my ($type, $suffix) = @$_;
		print {$c_fh} replace_variables($templates{c_functions}, type => $type, suffix => $suffix);
		print {$h_fh} replace_variables($templates{h_functions}, type => $type, suffix => $suffix);
	}
	print {$h_fh} $templates{h_footer};
	close($h_fh);
	close($c_fh);

	return;
}

sub replace_variables {
	my ($string, %variables) = @_;
	while (my ($variable, $value) = each(%variables)){
		$string =~ s/\$\Q$variable\E\$/$value/mg;
	}
	return $string;
}

sub read_templates {
	my %ret;
	my @data = <DATA>;
	my ($filename, $buffer);
	while (my $line = shift(@data)){
		if ($line =~ /^#####\s*(.*?)$/){
			if ($filename){
				$ret{$filename} = $buffer;
			}
			$filename = $1;
			$buffer = '';
		} else {
			$buffer .= $line;
		}
	}
	if ($filename){
		$ret{$filename} = $buffer;
	}
	return %ret;
}


unless (caller){
	exit(generate_alloc2d(@ARGV) || 0);
}

__DATA__
##### c_header
/** @file alloc_2d.c
	@brief Utility functions for allocating and freeing two-dimensional arrays of various types.

	This file was created by alloc_2d.pl and should not be edited manually.
*/

#include "alloc_2d.h"

#include <stdio.h>
#include <stdlib.h>


##### c_functions
$type$ **alloc2d$suffix$(size_t w, size_t h){
    $type$ **p = ($type$**) malloc(h * sizeof($type$*));
    if (p == NULL) {
        fprintf(stderr, "-E- %s line %d: Memory allocation failed.n", __FILE__, __LINE__);
        return NULL;
    }
    p[0] = ($type$*) malloc(w * h * sizeof($type$));
    if (p[0] == NULL) {
        fprintf(stderr, "-E- %s line %d: Memory allocation failed.n", __FILE__, __LINE__);
        free(p);
        return NULL;
    }
    for (size_t i = 1; i < h; i++) {
        p[i] = &(p[0][i * w]);
    }
    return p;
}
void free2d$suffix$($type$ **p) {
    free(p[0]);
    free(p);
}
void free2d_all$suffix$($type$ **p, size_t length) {
    for (size_t i = 0; i < length; i++) {
        free(p[i]);
    }
    free(p);
}

##### h_header
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

##### h_functions
/** @brief Allocate a two-dimensional array of type $type$ of a given size.

	The order of the parameters are the reverse of when the data is being accessed.  E.g., this is valid (ignoring the
	printf type specifier):

	$type$ **array = alloc_2d$suffix$(1, 5);
	printf("%d\n", array[4][0]);
	
	@param[in] w Width of array.  NB: this is the second array dimension.
	@param[in] h Height of array.  NB: this is the first array dimension.

	@return A malloc'd array or NULL if any malloc fails.
*/
$type$ **alloc2d$suffix$(size_t w, size_t h);
/** @brief Free a two-dimensional array created by alloc2d$suffix$.

	@param[in] p Pointer to array created by alloc2d$suffix$.
*/
void free2d$suffix$($type$ **p);
/** @brief Free a two-dimensional array created by malloc'ing each row individually.

	@param[in] p Pointer to array to free.
	@param[in] length Length of input array's first dimension.
*/
void free2d_all$suffix$($type$ **p, size_t length);

##### h_footer
#ifdef __cplusplus
}
#endif

#endif /* OEL_UTIL_LIBGENUTILS_ALLOC_2D_H_ */
