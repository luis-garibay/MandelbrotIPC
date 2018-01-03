CFLAGS=

mandelbrot: mandelbrot.c mandelCalc.c mandelDisplay.c
	gcc $(CFLAGS) -o mandelbrot mandelbrot.c
	gcc $(CFLAGS) -o mandelCalc mandelCalc.c
	gcc $(CFLAGS) -o mandelDisplay mandelDisplay.c

clean:
	rm mandelbrot mandelCalc mandelDisplay
