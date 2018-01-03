CFLAGS=

mandelbrot: mandelbrot-lgarib2.c mandelCalc-lgarib2.c mandelDisplay-lgarib2.c
	gcc $(CFLAGS) -o mandelbrot mandelbrot-lgarib2.c
	gcc $(CFLAGS) -o mandelCalc mandelCalc-lgarib2.c
	gcc $(CFLAGS) -o mandelDisplay mandelDisplay-lgarib2.c

clean:
	rm mandelbrot mandelCalc mandelDisplay
