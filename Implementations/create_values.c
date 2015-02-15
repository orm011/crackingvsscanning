#include "../Framework/distributions.h"
#include "../Framework/interface.h"

void create_values(const char *distribution, targetType* buffer, unsigned int size, targetType domain)
{
	if(strcmp("randomD", distribution) == 0)
		randomDistribution(buffer, size, domain, SEED);
	else if (strcmp("uniformD", distribution) == 0)
		uniformDistribution(buffer, size, domain, SEED);
	else if (strcmp("skewedD", distribution) == 0)
		skewedDistribution(buffer, size, domain, SEED, SKEW);
	else if (strcmp("holgerD", distribution) == 0)
		holgerDistribution(buffer, size);
	else if (strcmp("sortedD", distribution) == 0)
		sortedData(buffer, size, domain, SEED);
	else if (strcmp("revsortedD", distribution) == 0)
		revsortedData(buffer, size, domain, SEED);
	else if (strcmp("almostsortedD", distribution) == 0)
		almostsortedData(buffer, size, domain, SEED, 10);
	else
	{
		fprintf(stderr,"Wrong distribution name.\n");
		exit (1);
	}
}
