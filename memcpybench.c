#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <sys/time.h>
#include <getopt.h>

/* lifted from http://stackoverflow.com/questions/1715224/very-fast-memcpy-for-image-processing
 */
/* void X_aligned_memcpy_sse2(void* dest, const void* src, const unsigned long size_t)
{

  __asm
  {
    mov esi, src;    //src pointer
    mov edi, dest;   //dest pointer

    mov ebx, size_t; //ebx is our counter 
    shr ebx, 7;      //divide by 128 (8 * 128bit registers)


    loop_copy:
      prefetchnta 128[ESI]; //SSE2 prefetch
      prefetchnta 160[ESI];
      prefetchnta 192[ESI];
      prefetchnta 224[ESI];

      movdqa xmm0, 0[ESI]; //move data from src to registers
      movdqa xmm1, 16[ESI];
      movdqa xmm2, 32[ESI];
      movdqa xmm3, 48[ESI];
      movdqa xmm4, 64[ESI];
      movdqa xmm5, 80[ESI];
      movdqa xmm6, 96[ESI];
      movdqa xmm7, 112[ESI];

      movntdq 0[EDI], xmm0; //move data from registers to dest
      movntdq 16[EDI], xmm1;
      movntdq 32[EDI], xmm2;
      movntdq 48[EDI], xmm3;
      movntdq 64[EDI], xmm4;
      movntdq 80[EDI], xmm5;
      movntdq 96[EDI], xmm6;
      movntdq 112[EDI], xmm7;

      add esi, 128;
      add edi, 128;
      dec ebx;

      jnz loop_copy; //loop please
    loop_copy_end:
  }
}
*/

long timediff(struct timeval before, struct timeval after){
  return (after.tv_usec - before.tv_usec) + (after.tv_sec-before.tv_sec)*1000000;
}


int main( int argc, char ** argv) {

  long sizemb = -1;
  const char * algo;
  char c = -1;

  while ((c = getopt (argc, argv, "s:a:")) != -1) {
    switch (c) {
    case 's':
      sizemb = atoi(optarg);
      break;
    case 'a':
      algo = optarg;
      break;
    default:
      assert(("input error", 0));
    }  }

  assert(sizemb > 0);

  size_t num = ((size_t)sizemb) * 1024 * 1024 / sizeof(char);
  const char * src = (char*)malloc(num);
  char * dst = (char*)malloc(num);

  struct timeval before, after;
  gettimeofday(&before, NULL);

  if (strcmp(algo,"memmove") == 0 ) {
    memmove(dst, src, num);
  } else if (strcmp(algo, "memcpy" ) == 0) {
    memcpy(dst, src, num);
  } else if (strcmp(algo, "builtin") == 0) {
    __builtin_memcpy(dst, src, num); 
  } else {
    assert(("invalid algo", 0));
  }

  gettimeofday(&after, NULL);

  long diff = timediff(before, after);
  printf("{\"sizemb\": %ld, \"timemilli\": %ld}\n", sizemb, diff/1000);
}
