import sys, os
import Image, ImageDraw

# this script takes an RGB picture and cuts it into frames for a scrolling video

# the spectrogram.png file has to be RGB mode (watch out, the spectrogram program generates grayscale by default) and resized to 720 pixels height in advance.

# a video from the frames can be generated with ffmpeg:
# ffmpeg -f image2 -r 25 -i ./%06d.png -i soundfile.mp3 -b 5000k -vb 5000k -acodec copy out.mp4

def main():
    i = Image.open("spectrogram-rgb.png")
    res = (1280,720)
    # step size 2 means 25 fps for spectrogram with 50 pixels per second
    step = 4
    for center in xrange(step*0,i.size[0], step):
        left = center-res[0]/2
        left = max(left, 0)
        right = left+res[0]
        if right > i.size[0]-1:
            right = i.size[0]-1
            left = right-res[0]
        o = i.crop((left, 0, right, res[1]))
        draw = ImageDraw.Draw(o)
        xoff = center-left
        draw.line((center-left-1, 0, center-left-1, res[1]), (0,0,180))
        draw.line((center-left, 0, center-left, res[1]), (0,0,180))
        draw.line((center-left+1, 0, center-left+1, res[1]), (0,0,180))
        name = str(center/step).rjust(6, "0")
        o.save("%s.png"%name)
        if center%10 == 0:
            print "crop %s.png"%name

main()
