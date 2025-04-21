all: time audiostreams audiostreamc
time:
	@echo "-----------------------------------"
	@echo "| $(shell date) |"
	@echo "-----------------------------------"
audiostreams: audiostreams.c
	gcc -o audiostreams audiostreams.c
audiostreamc: audiostreamc.c
	gcc -o audiostreamc audiostreamc.c
clean:
	rm audiostreams audiostreams