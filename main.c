#include "switch.h"

int main(int argc, char **argv)
{
	int ret = 0;
	struct Switch sw;

	ret = switch_init(&sw, argc, argv);
	ret = switch_run();
	switch_stop(&sw);

	return ret;
}