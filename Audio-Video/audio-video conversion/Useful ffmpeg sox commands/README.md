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

# converts a wav ﬁle to a 16-bit signed integer mono ﬁle with 8kHz wav
sox infile.wav -b 16 -e signed-integer -c 1 -r 8k outfile.wav

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
<br>

## Trim 
<br>

``` bash
#sox input output trim <start> <duration> 
#If you want to make 10 seconds audio file from the start
sox Input.wav Output.wav trim 0 10

#If you want to make 10 seconds audio file from the 5second
sox Input.wav Output.wav trim 5 10

```

<br><br>

# Useful ffmpeg commands

<br>

## make mp4 from a single png image

<br>

``` bash
# convert to 15 seconds, (320X240)  mp4 file
ffmpeg -loop 1 -i input.png -c:v libx264 -t 15 -pix_fmt yuv420p -vf scale=320:240 output.mp4
# convert to 15 seconds, (640X480)  mp4 file
ffmpeg -loop 1 -i input.png -c:v libx264 -t 15 -pix_fmt yuv420p -vf scale=640:480 output.mp4
```
<br>

## make webm from mp4

<br>

webm is an extension of google's vp codec video. It is mainly encoded with VP8 and VP9 codecs.

<br>

```bash
# convert to vp8 webm file from mp4
ffmpeg -i input.mp4 -c:v libvpx -b:v 1M  output.webm -y

# convert to vp8 webm file from mp4
ffmpeg -i input.mp4 -c:v libvpx-vp9 -b:v 1M output.webm -y
```

<br>

## Adding or replacing audio track 

<br>
This command adds a new audio track if the video file does not already have one. If an audio track already exists, the existing audio track is removed and replaced with new audio (input.wav).

<br>

``` bash
ffmpeg  -i input.mp4 -i input.wav -c:v copy -map 0:v -map 1:a -y output.mp4
```

<br>

If the video and audio lengths are different, add the '-shortest' option to add the audio or video based on the shorter length.

<br>

``` bash
ffmpeg  -i input.mp4 -i input.wav -c:v copy -map 0:v -map 1:a -y -shortest output.mp4
```
