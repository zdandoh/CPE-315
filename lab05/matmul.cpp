#include <stdlib.h>
#include <stdio.h>
#include "matmul.h"

Matrix Allocate2ndMatrix(int height, int width, int init);

void matmul( float*, const float*, const float*, unsigned int, unsigned int, unsigned int);

////////////////////////////////////////////////////////////////////////////////
//! C = A * B
//! @param C          result matrix
//! @param A          matrix A 
//! @param B          matrix B
//! @param hA         height of matrix A
//! @param wB         width of matrix B
////////////////////////////////////////////////////////////////////////////////

void matmul_sub(float* C, const float* A, const float* B, unsigned int sub_size, 
  unsigned int big_size)
{
  for (unsigned int i = 0; i < sub_size; ++i)
    for (unsigned int j = 0; j < sub_size; ++j) {
      double sum = 0;
      for (unsigned int k = 0; k < sub_size; ++k) {
        double a = A[i * big_size + k];
        double b = B[j * big_size + k];
        sum += a * b;
      }
      C[i * big_size + j] += (float)sum;
    }
}

/* You'll need to modify this function such that matrix B is accessed
 * correctly once you change the memory layout to column-major. */
void matmul(float* C, const float* A, const float* B, unsigned int hA, 
    unsigned int wA, unsigned int wB)
{
  unsigned int total = wA * hA;
  for(unsigned int k = 0; k < total; k += wA * BLOCK_SIZE) {
    for(unsigned int i = 0; i < wA; i += BLOCK_SIZE) {
      for(unsigned int j = 0; j < wA; j += BLOCK_SIZE) {
        matmul_sub(C + k + i, A + k + j, B + (i * wA) + j, BLOCK_SIZE, wB);
      }
    }
  }
}

// Allocate a matrix of dimensions height*width
Matrix Allocate2ndMatrix(int height, int width)
{
  Matrix M;
  M.width = M.pitch = width;
  M.height = height;
  int size = M.width * M.height;
  M.elements = NULL;

  M.elements = (float*) malloc(size*sizeof(float));

  /* This is a row-major allocation and initialization.
   * You need to modify this function which is only called
   * for Matrix B such that a column-major ordering is
   * performed. */
  unsigned int count = 0;
  unsigned int line_ct = 0;
  for(unsigned int i = 0; i < M.height * M.width; i++)
  {
    if(i % M.width == 0 && i != 0) {
      count = 0;
      line_ct++;
    }
    M.elements[count * M.width + line_ct] = (rand() / (float)RAND_MAX);
    count++;
  }
  return M;
}