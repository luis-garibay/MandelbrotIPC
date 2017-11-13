# Luis Garibay ; lgaribay

mandelbrot: mandelbrot-lgarib2.c mandelCalc-lgarib2.c mandelDisplay-lgarib2.c
	gcc -o mandelbrot mandelbrot-lgarib2.c
	gcc -o mandelCalc mandelCalc-lgarib2.c
	gcc -o mandelDisplay mandelDisplay-lgarib2.c

clean:
	rm mandelbrot mandelCalc mandelDisplay
