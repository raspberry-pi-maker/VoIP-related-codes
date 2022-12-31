# G711 to PCM

``` bash
#compile the code
g++ main.cpp -o g711_2_pcm
g++ g++ linear_g711.cpp -o linear_g711
```
<br>
then run the code like this. 
This code creates 8000Hz, 16bit, mono PCM files.

``` bash
./g711_2_pcm -i input.alaw -o output.pcm -c alaw
./g711_2_pcm -i input.ulaw -o output.pcm -c mulaw

./linear_g711 -i input.pcm -o output.alaw -c alaw
./linear_g711 -i input.pcm -o output.ulaw -c mulaw
```

 
