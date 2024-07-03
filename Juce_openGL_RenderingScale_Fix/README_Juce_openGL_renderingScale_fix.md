# This patch fixes a JUCE v7 openGL RenderingScale bug:
"OpenGLContext: Make getRenderingScale() sensitive to Component transforms"  
https://github.com/juce-framework/JUCE/commit/b0167985b4648bd950d999ee7849d31938b3a54a  

Note: This commit is on the JUCE develop branch  
And is now rolled into JUCE v8  
So if you're using JUCE v8 and above, you won't need to apply this patch  

# Apply the Juce_openGL_renderingScale_fix.patch to your Juce source:
Copy this patch to your <juceSource>  
```
cd <juceSource>  
git apply Juce_openGL_renderingScale_fix.patch
```
