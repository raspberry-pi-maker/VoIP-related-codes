# Useful sox commands for audio file

## file information
<br>

``` bash
# Output information from an audio file with a header
sox --i infile.wav

# converts a wav ﬁle to a raw 16-bit signed integer mono ﬁle with 8kHz
sox infile.wav -b 16 -e signed-integer -c 1 -r 8k -t raw outfile.pcm

# converts a pcm ﬁle back to WAV
sox -b 16 -e signed-integer -c 1 -r 8k infile.pcm outfile.wav


```
<br>


## format conversion
<br>

``` bash
# converts the samplerate to 44100 
sox infile.wav -r 44100  outfile.wav

# converts a wav ﬁle to a raw 16-bit signed integer mono ﬁle with 8kHz
sox infile.wav -t raw outfile.pcm  # Maintain wav file format

# converts a wav ﬁle to a raw 16-bit signed integer mono ﬁle with 8kHz
sox infile.wav -b 16 -e signed-integer -c 1 -r 8k -t raw outfile.pcm

sox stereo.wav left.wav remix 1 # convert to mono file by selecting left channel 
sox stereo.wav right.wav remix 2 # convert to mono file by selecting right channel 
sox stereo.wav -c 1 mix.wav  # convert to mono file by mixing 2 channels
```
<br>


## merge 2 mono files into one stereo file
<br>

``` bash
sox -M -c 1 left.wav -c 1 right.wav output.wav 
sox left.wav right.wav -c 2 output.wav -M
```
<br>
 
## Split stereo file into two mono files
<br>

``` bash
sox stereo.wav outfile.l.wav remix 1    # left channel
sox stereo.wav outfile.r.wav remix 2    # right channel
```
<br>

## Extract Subparts of a File
<br>

``` bash
sox Input.wav  Half1.wav trim 0:0 0:30 # 0 ~ 30 seconds
```
<br>

## Concatenate  files
<br>

``` bash
sox Half1.wav Half2.wav Full.wav
```

