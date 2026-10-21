// One physical material family drives the field console, pages and popups.
// Part order: case, art well, key, tab, calm field, gauge, gauge fill,
// opaque lock, adjustment wheel, handbook, research instrument.
UTexture2D* ConsoleTextureV79(int32 Part) const
{return Artwork().ConsoleV79[ConsoleNationIndexV82][FMath::Clamp(Part,0,12)].Get();}
FLinearColor ConsoleTintV79(float Level=1.f) const
{
    const FLinearColor Tints[]={FLinearColor(.66f,.67f,.65f,1),FLinearColor(.69f,.71f,.72f,1),FLinearColor(.69f,.72f,.74f,1),FLinearColor(.67f,.67f,.63f,1)};
    return Tints[ConsoleNationIndexV82]*FLinearColor(Level,Level,Level,1);
}
void ConsoleFieldV79(float X,float Y,float W,float H,FLinearColor Tint,int32 FacePart=4) const
{
    auto* T=ConsoleTextureV79(FacePart);if(!FrontlineTextureReady(T))return;
    // Native-scale fine enamel avoids stretching a small field over a 4K page.
    // Adjacent mirrored edges share texels; no large authored motif is repeated.
    constexpr float Tile=256.f;
    const float U0=FacePart==1?136.f/T->GetSizeX():0.f,V0=FacePart==1?136.f/T->GetSizeY():0.f;
    const float US=FacePart==1?354.f/T->GetSizeX():1.f,VS=FacePart==1?354.f/T->GetSizeY():1.f;
    int32 Row=0;
    for(float YP=Y;YP<Y+H-.01f;YP+=Tile,++Row)
    {
        const float TH=FMath::Min(Tile,Y+H-YP);int32 Col=0;
        for(float XP=X;XP<X+W-.01f;XP+=Tile,++Col)
        {
            const float TW=FMath::Min(Tile,X+W-XP);
            TextureV83(T,OX+XP*Scale,OY+YP*Scale,TW*Scale,TH*Scale,
                U0+(Col%2?US:0.f),V0+(Row%2?VS:0.f),(Col%2?-1.f:1.f)*US*TW/Tile,(Row%2?-1.f:1.f)*VS*TH/Tile,Tint);
        }
    }
}
bool ConsoleSliceV80(int32 Part,float X,float Y,float W,float H,float Edge,FLinearColor Tint) const
{
    auto* T=ConsoleTextureV79(Part);if(!FrontlineTextureReady(T)||W<=0||H<=0)return false;
    if(Part==4){ConsoleFieldV79(X,Y,W,H,Tint);return true;}
    const float SW=T->GetSizeX(),SH=T->GetSizeY();
    if(Part==5)
    {
        // A shallow long gauge is a three-slice housing. Its complete end cap
        // keeps the native aspect ratio; the full-height central rail tiles.
        // Nine-slicing this row repeated the middle of its screws vertically.
        const float SX=160.f,EX=FMath::Min(W*.25f,SX*H/SH),Tile=(SW-2*SX)*H/SH;
        for(int32 Side=0;Side<2;++Side)TextureV83(T,OX+(Side?X+W-EX:X)*Scale,OY+Y*Scale,EX*Scale,H*Scale,Side?1-SX/SW:0,0,SX/SW,1,Tint);
        int32 I=0;for(float XP=X+EX;XP<X+W-EX-.01f;XP+=Tile,++I)
        {const float TW=FMath::Min(Tile,X+W-EX-XP);TextureV83(T,OX+XP*Scale,OY+Y*Scale,TW*Scale,H*Scale,I%2?1-SX/SW:SX/SW,0,(I%2?-1.f:1.f)*(1-2*SX/SW)*TW/Tile,1,Tint);}
        return true;
    }

    // Measured asset-specific source pixels. The protected rectangles contain
    // the COMPLETE shoe and inner bend, rather than cutting through a chamfer.
    const float SourceCuts[4][6]={{168,112,144,104,0,64},{168,132,136,132,0,64},
        {230,240,170,165,0,64},{230,240,185,165,0,64}};
    const int32 CutNation=ConsoleNationIndexV82>=2&&!T->GetPathName().StartsWith(TEXT("/Game/FRRUIntegration/FormalV82/UI/Console/"))?(bGermanFinish?1:0):ConsoleNationIndexV82;
    float SX=Part==5?160.f:SourceCuts[CutNation][FMath::Clamp(Part,0,5)],SY=Part==5?64.f:SX;
    SX=FMath::Min(SX,SW*.40f);SY=FMath::Min(SY,SH*.40f);
    const float E=FMath::Min(Edge,FMath::Min(W,H)*.35f);
    const float EX=Part==5?FMath::Min(W*.25f,E*SX/SY):E;
    const float DX[]={X,X+EX,X+W-EX,X+W},DY[]={Y,Y+E,Y+H-E,Y+H};
    const float U[]={0,SX/SW,1-SX/SW,1},V[]={0,SY/SH,1-SY/SH,1};
    // Paint is sampled at its own scale. No enamel face or wear pattern is
    // stretched into a wide header, a shallow key or a long frame.
    ConsoleFieldV79(X,Y,W,H,Tint);
    for(int32 R:{0,2})for(int32 C:{0,2})
    {
        const float DW=DX[C+1]-DX[C],DH=DY[R+1]-DY[R];
        TextureV83(T,OX+DX[C]*Scale,OY+DY[R]*Scale,DW*Scale,DH*Scale,U[C],V[R],U[C+1]-U[C],V[R+1]-V[R],Tint);
    }
    // The long axis tiles at the corner's texel scale. Alternating mirroring
    // shares exact seam texels; a final partial tile crops rather than stretches.
    const float TileW=FMath::Max(1.f,(SW-2*SX)*EX/SX),TileH=FMath::Max(1.f,(SH-2*SY)*E/SY);
    int32 I=0;
    for(float XP=X+EX;XP<X+W-EX-.01f;XP+=TileW,++I)
    {
        const float TW=FMath::Min(TileW,X+W-EX-XP),SU=I%2?U[2]:U[1],DU=(I%2?-1.f:1.f)*(U[2]-U[1])*TW/TileW;
        for(int32 R:{0,2})TextureV83(T,OX+XP*Scale,OY+DY[R]*Scale,TW*Scale,E*Scale,SU,V[R],DU,V[R+1]-V[R],Tint);
    }
    I=0;
    for(float YP=Y+E;YP<Y+H-E-.01f;YP+=TileH,++I)
    {
        const float TH=FMath::Min(TileH,Y+H-E-YP),SV=I%2?V[2]:V[1],DV=(I%2?-1.f:1.f)*(V[2]-V[1])*TH/TileH;
        for(int32 C:{0,2})TextureV83(T,OX+DX[C]*Scale,OY+YP*Scale,EX*Scale,TH*Scale,U[C],SV,U[C+1]-U[C],DV,Tint);
    }
    return true;
}
bool ConsoleSkinV79(int32 Slot,float X,float Y,float W,float H,float Edge,FLinearColor Tint) const
{
    const int32 Part=Slot==0||Slot==1?0:Slot==3?2:1;
    return ConsoleSliceV80(Part,X,Y,W,H,FMath::Max(Edge,Slot==0?10.f:Slot==3?10.f:8.f),ConsoleTintV79()*Tint);
}
bool ConsoleToolV79(int32 Part,float X,float Y,float W,float H) const
{
    auto* Texture=ConsoleTextureV79(Part);
    if(!FrontlineTextureReady(Texture))return false;
    ImageContain(Texture,X,Y,W,H,Part>=11?FLinearColor(.76f,.76f,.73f,1):FLinearColor(.83f,.84f,.80f,1));return true;
}
void ConsoleSelectionV79(float X,float Y,float W,float H,bool Selected,bool Hover=false) const
{
    if(!Selected||W<14||H<14)return;
    // Selected keys depress an inner lip. Hover uses the real face's lighting;
    // no extra miniature plaque can escape the button or sit over its caption.
    const float Inset=FMath::Min(7.f,FMath::Min(W,H)*.18f);
    const auto C=bGermanFinish?FLinearColor(.45f,.46f,.43f,.38f):FLinearColor(.48f,.42f,.28f,.42f);
    Line(X+Inset,Y+H-Inset,X+W-Inset,Y+H-Inset,C,1.f);
    Line(X+Inset,Y+Inset,X+W-Inset,Y+Inset,FLinearColor(0,0,0,.48f),1.f);
}
void ConsoleArtFrameV79(float X,float Y,float W,float H,bool Hover=false,bool Selected=false) const
{
    PageShadowV74(X-5,Y-5,W+10,H+10,3);
    Skin(2,X-5,Y-5,W+10,H+10,6,Hover?FLinearColor(1.10f,1.10f,1.08f,1):FLinearColor::White);
    ConsoleSelectionV79(X-5,Y-5,W+10,H+10,Selected,Hover);
}
void ConsoleSeamV79(float X,float Y,float EX,float EY,const FLinearColor& Ink,float Width=1.f) const
{
    // Routed dark groove, faint lower lip, and subdued paint in the channel.
    // A dependency remains legible without looking like a vector overlay.
    Line(X,Y,EX,EY,FLinearColor(.003f,.004f,.004f,.72f),Width+2.f);
    Line(X+.65f,Y+.65f,EX+.65f,EY+.65f,bGermanFinish?FLinearColor(.28f,.30f,.31f,.22f):FLinearColor(.35f,.30f,.20f,.22f),Width+1.f);
    Line(X,Y,EX,EY,FLinearColor(Ink.R,Ink.G,Ink.B,Ink.A*.60f),Width);
}
void ConsoleGaugeV79(float X,float Y,float W,float H) const
{
    if(!ConsoleSliceV80(5,X,Y,W,H,6,ConsoleTintV79()))Skin(2,X,Y,W,H,6);
}
void ConsoleFillV79(float X,float Y,float FullWidth,float H,float Fraction,bool Left,const FLinearColor& Tint,bool FieldReadout=false,float FieldPaintLevel=2.2f,int32 Nation=INDEX_NONE) const
{
    Fraction=FMath::Clamp(Fraction,0.f,1.f);const float W=FullWidth*Fraction;
    if(W<=.1f)return;
    Nation=Nation==INDEX_NONE?ConsoleNationIndexV82:FMath::Clamp(Nation,0,3);
    auto* T=Artwork().ConsoleV79[Nation][6].Get();
    if(!FrontlineTextureReady(T))return;
    // Reveal fixed UVs instead of compressing paint and scale marks at low values.
    const float PX=Left?X:X+FullWidth-W,U=Left?0.f:1.f-Fraction;
    // Field instruments need readable worn paint against the recessed housing.
    // Keep the original texture detail; sliders retain their existing finish.
    const float Peak=FMath::Max(.01f,FMath::Max3(Tint.R,Tint.G,Tint.B));
    const FLinearColor DrawTint=FieldReadout?FLinearColor(FieldPaintLevel*Tint.R/Peak,FieldPaintLevel*Tint.G/Peak,FieldPaintLevel*Tint.B/Peak,Tint.A):FLinearColor(.90f,.90f,.86f,1);
    // Neutralize the old amber/olive paint before applying national enamel.
    // Each army owns an immutable texture instance; the opposing gauge never
    // inherits the player's console paint or mutates an already queued tile.
    const int32 Paint=FieldPaintLevel>1.8f?0:FieldPaintLevel>1.25f?2:1;
    if(FieldReadout&&Artwork().ConsolePigmentsV8213[Nation][Paint])
        MaterialTile(Artwork().ConsolePigmentsV8213[Nation][Paint],PX,Y,W,H,T->GetResource(),U,Paint?.60f:.12f,Fraction,.28f,FLinearColor(1,1,1,Tint.A));
    else TextureV83(T,OX+PX*Scale,OY+Y*Scale,W*Scale,H*Scale,U,Tint.B>Tint.R?.12f:.60f,Fraction,.28f,DrawTint);
    Gradient(PX,Y,W,H,FLinearColor(0,0,0,FieldReadout?.04f:.08f),FLinearColor(0,0,0,FieldReadout?.16f:.30f),true);
    Line(PX,Y+H-.5f,PX+W,Y+H-.5f,FLinearColor(.006f,.008f,.007f,FieldReadout?.50f:.75f),.75f);
}
