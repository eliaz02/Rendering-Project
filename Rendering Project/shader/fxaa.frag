#version 330 core

in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D screenTexture;
uniform vec2 rcpFrame; // Reciprocal of the frame size: (1.0/width, 1.0/height)

// FXAA Quality Settings
#define FXAA_QUALITY_PRESET 12
#define FXAA_EDGE_THRESHOLD      (1.0/8.0)      // NVIDA FXAA_WhitePaper high quality
#define FXAA_EDGE_THRESHOLD_MIN  (1.0/16.0)     // NVIDA FXAA_WhitePaper high quality
#define FXAA_SEARCH_STEPS        16
#define FXAA_SEARCH_THRESHOLD    (1.0/4.0)
#define FXAA_SUBPIX_TRIM         (1.0/4.0)      // NVIDA FXAA_WhitePaper default removal
#define FXAA_SUBPIX_TRIM_SCALE   (1.0 - FXAA_SUBPIX_TRIM)
#define FXAA_SUBPIX_CAP          (3.0/4.0)      // NVIDA FXAA_WhitePaper default ammount

float FxaaLuma(vec3 rgb) {
    return rgb.y * (0.587/0.299) + rgb.x;
}

vec3 FxaaPixelShader(vec2 pos, sampler2D tex, vec2 rcpFrame) {

    // compute the neighbors pixel
    vec3 rgbN = texture(tex, pos + vec2(0.0, -rcpFrame.y)).rgb;
    vec3 rgbW = texture(tex, pos + vec2(-rcpFrame.x, 0.0)).rgb;
    vec3 rgbE = texture(tex, pos + vec2(rcpFrame.x, 0.0)).rgb;
    vec3 rgbS = texture(tex, pos + vec2(0.0, rcpFrame.y)).rgb;
    vec3 rgbM = texture(tex, pos).rgb;

    // compute the luminance of the pixel
    float lumaN = FxaaLuma(rgbN);
    float lumaW = FxaaLuma(rgbW);
    float lumaM = FxaaLuma(rgbM);
    float lumaE = FxaaLuma(rgbE);
    float lumaS = FxaaLuma(rgbS);
    
    // compute the max luminance difference betwin the pixel
    float rangeMin = min(lumaM, min(min(lumaN, lumaW), min(lumaS, lumaE)));
    float rangeMax = max(lumaM, max(max(lumaN, lumaW), max(lumaS, lumaE)));
    float range = rangeMax - rangeMin;
    
    // discad the edge if the the contrast is not big enought
    if(range < max(FXAA_EDGE_THRESHOLD_MIN, rangeMax * FXAA_EDGE_THRESHOLD))  return rgbM;

    // Diagonal samples
    vec3 rgbNW = texture(tex, pos + vec2(-rcpFrame.x, -rcpFrame.y)).rgb;
    vec3 rgbNE = texture(tex, pos + vec2(rcpFrame.x, -rcpFrame.y)).rgb;
    vec3 rgbSW = texture(tex, pos + vec2(-rcpFrame.x, rcpFrame.y)).rgb;
    vec3 rgbSE = texture(tex, pos + vec2(rcpFrame.x, rcpFrame.y)).rgb;
    
    // compute the avarege color of the square around at the pixel
    vec3 rgbL = (rgbNW + rgbNE + rgbSW + rgbSE + rgbN + rgbW + rgbM + rgbE + rgbS);
    rgbL *= (1.0/9.0);

    // compute the luminance of the pixel
    float lumaNW = FxaaLuma(rgbNW);
    float lumaNE = FxaaLuma(rgbNE);
    float lumaSW = FxaaLuma(rgbSW);
    float lumaSE = FxaaLuma(rgbSE);
    
    // compute the direction of the edge 
    float edgeVert = 
        abs((0.25 * lumaNW) + (-0.5 * lumaN) + (0.25 * lumaNE)) +
        abs((0.50 * lumaW ) + (-1.0 * lumaM) + (0.50 * lumaE )) +
        abs((0.25 * lumaSW) + (-0.5 * lumaS) + (0.25 * lumaSE));
        
    float edgeHorz = 
        abs((0.25 * lumaNW) + (-0.5 * lumaW) + (0.25 * lumaSW)) +
        abs((0.50 * lumaN ) + (-1.0 * lumaM) + (0.50 * lumaS )) +
        abs((0.25 * lumaNE) + (-0.5 * lumaE) + (0.25 * lumaSE));
        
    bool horzSpan = edgeHorz >= edgeVert;
    
    
    // change coordinate to work just to north and south
    if(!horzSpan) {
        lumaN = lumaW;
        lumaS = lumaE;
    }
    
    float gradientN = abs(lumaN - lumaM);
    float gradientS = abs(lumaS - lumaM);
    lumaN = (lumaN + lumaM) * 0.5;
    lumaS = (lumaS + lumaM) * 0.5;

    // change the direction to work just to north
    bool pairN = gradientN >= gradientS;
    float lengthSign = horzSpan ? -rcpFrame.y : -rcpFrame.x;
    if(!pairN) {
        lumaN = lumaS;
        gradientN = gradientS;
        lengthSign *= -1.0;
    }
    
    // posN (negative) posP (positive) is like head of the hard disk that we use to seek the stard and end of the edge  
    vec2 posN;
    posN.x = pos.x + (horzSpan ? 0.0 : lengthSign * 0.5);
    posN.y = pos.y + (horzSpan ? lengthSign * 0.5 : 0.0);
    
    // bound o th search 
    gradientN *= FXAA_SEARCH_THRESHOLD;
    
    vec2 posP = posN;

    // step of the search 
    vec2 offNP = horzSpan ? vec2(rcpFrame.x, 0.0) : vec2(0.0, rcpFrame.y);
    float lumaEndN = lumaN;
    float lumaEndP = lumaN;
    
    bool doneN = false;
    bool doneP = false;
    posN += offNP * vec2(-1.0, -1.0);
    posP += offNP * vec2( 1.0,  1.0);
    
    // End-of-edge Search 
    for(int i = 0; i < FXAA_SEARCH_STEPS; i++) {
        // If the search in a direction isn't done, sample the luminance at the current position
        if(!doneN) {
            lumaEndN = FxaaLuma(texture(tex, posN).rgb);
        }
        if(!doneP) {
            lumaEndP = FxaaLuma(texture(tex, posP).rgb);
        }
        // If the luminance difference exceeds gradientN, the edge endpoint is found
        doneN = doneN || (abs(lumaEndN - lumaN) >= gradientN);
        doneP = doneP || (abs(lumaEndP - lumaN) >= gradientN);
        
        // Exit early
        if(doneN && doneP) break;
        
        // Moving along the edge
        if(!doneN) posN -= offNP;
        if(!doneP) posP += offNP;
    }
    // compute the distance in pixel to the negative and positive edge points
    float dstN = horzSpan ? pos.x - posN.x : pos.y - posN.y;
    float dstP = horzSpan ? posP.x - pos.x : posP.y - pos.y;
    // compute the Dominant Edge Direction 
    bool directionN = dstN < dstP;
    lumaEndN = directionN ? lumaEndN : lumaEndP;

    // Checks if the edge contrast direction (darker to brighter) is consistent
    // If not, sets lengthSign = 0.0 (disables blending offset to avoid artifacts).
    if(((lumaM - lumaN) < 0.0) == ((lumaEndN - lumaN) < 0.0)) {
        lengthSign = 0.0;
    }
    
    // Compute Sub-Pixel Offset
    float spanLength = dstP + dstN;
    dstN = directionN ? dstN : dstP;
    float subPixelOffset = (0.5 + (dstN * (-1.0/spanLength))) * lengthSign;
    
    // Fech the color for the blending aka far rgb 
    vec3 rgbF = texture(tex, vec2(
        pos.x + (horzSpan ? 0.0 : subPixelOffset),
        pos.y + (horzSpan ? subPixelOffset : 0.0))).rgb;

    // Use local avarege luma, far luma for comute the scale of mixing  
    float lumaF = FxaaLuma(rgbF);
    float lumaB = mix(lumaF, FxaaLuma(rgbL), FXAA_SUBPIX_TRIM_SCALE);
    float scale = min(FXAA_SUBPIX_CAP, max(0.0, (abs(lumaF - lumaM) / range) - FXAA_SUBPIX_TRIM));
    
    // Mixing 
    return mix(rgbF, rgbL, scale * FXAA_SUBPIX_TRIM_SCALE);
}

void main() {
    vec3 color = FxaaPixelShader(TexCoord, screenTexture, rcpFrame);
    FragColor = vec4(color, 1.0);
}