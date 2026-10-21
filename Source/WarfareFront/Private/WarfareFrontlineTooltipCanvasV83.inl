    mutable FTooltipStateV83* TooltipStateV83=nullptr;
    // Value-only, same-frame foreground buffers for museum/war-room popups.
    // Field tooltip motion and retained snapshots keep their existing path.
    mutable TArray<FTooltipBufferV83> PageTooltipsV8222;
    mutable TArray<FTooltipOpV83>* RecordingV83=nullptr;
    mutable bool MotionStableV83=false,MotionFieldV83=false;
    mutable double MotionNumberAgeV83=0;
    mutable float ReplayAlphaV83=1,ReplayYV83=0,ReplayXV83=0;
    FLinearColor MotionInkV83(FLinearColor C)const {C.A*=ReplayAlphaV83;return C;}
    FTooltipOpV83& OpV83(ETooltipOpV83 Kind,float X,float Y,float W,float H,FLinearColor Ink)const
    {auto& O=RecordingV83->AddDefaulted_GetRef();O.Kind=Kind;O.X=X;O.Y=Y;O.W=W;O.H=H;O.Ink=Ink;return O;}
    void StartTooltipsV83(bool Field)const
    {
        MotionFieldV83=Field;
        auto& S=ScreenState(HUD.GetOwningPlayerController()).TooltipV83;TooltipStateV83=&S;
        const int32 Language=int32(FrontlineLocalizationV35::GetLanguage());
        const auto* B=Cast<AWarfareFrontlineGameMode>(HUD.GetWorld()->GetAuthGameMode());
        const FGuid Encounter=B?B->GetEncounterId():FGuid();
        const float DPI=Surface->Canvas->GetDPIScale();UFont* ControlFace=Artwork().ControlRuntimeV62.Get();
        // A retained snapshot contains fitted rows and raster font sizes. A
        // native DPI/Canvas route or face change must refit it once, even when
        // the output resolution, language and hovered object are unchanged.
        if(!Field||S.OutputScale!=Scale||S.CanvasDPI!=DPI||S.HDRComposite!=bHDRComposite
            ||S.LatinFace.Get()!=Font||S.ChineseFace.Get()!=ChineseFont||S.ControlFace.Get()!=ControlFace
            ||S.Language!=Language||S.Encounter!=Encounter)
        {
            S.Reset();S.OutputScale=Scale;S.CanvasDPI=DPI;S.HDRComposite=bHDRComposite;
            S.LatinFace=Font;S.ChineseFace=ChineseFont;S.ControlFace=ControlFace;
            S.Language=Language;S.Encounter=Encounter;
        }
        S.Candidate=0;
        PageTooltipsV8222.Reset();
    }
    template<typename Builder> void PageTooltipV8222(uint64 Key,uint64 Revision,Builder&& Build,FVector2D Anchor=FVector2D::ZeroVector)const
    {
        // A nested decoration stays in its parent's foreground snapshot;
        // growing PageTooltips here would invalidate the parent's op buffer.
        if(RecordingV83){Build();return;}
        if(MotionFieldV83){TooltipV83(Key,Revision,Forward<Builder>(Build),Anchor);return;}
        TGuardValue<bool> LegacyScopeV8210(bLegacyTypographyV8210,false);
        TGuardValue<bool> TooltipScopeV8210(bTooltipTypographyV8210,true);
        auto& Buffer=PageTooltipsV8222.AddDefaulted_GetRef();Buffer.Ready=true;
        Buffer.Revision=Revision;Buffer.CapturedAnchor=Buffer.LastAnchor=Anchor;
        auto* Previous=RecordingV83;RecordingV83=&Buffer.Ops;Build();RecordingV83=Previous;
    }
    template<typename Builder> void TooltipV83(uint64 Key,uint64 Revision,Builder&& Build,FVector2D Anchor=FVector2D::ZeroVector,float FollowBottom=0)const
    {
        TGuardValue<bool> LegacyScopeV8210(bLegacyTypographyV8210,false);
        TGuardValue<bool> TooltipScopeV8210(bTooltipTypographyV8210,true);
        if(!MotionFieldV83||MotionStableV83){Build();return;}
        auto& S=*TooltipStateV83;S.Candidate=Key;
        const double Now=FPlatformTime::Seconds();
        if(UIMotionReviewV83::TooltipAge>=0){S.Pending=Key;S.HoverAt=Now-UIMotionReviewV83::TooltipAge;}
        if(S.Pending!=Key)return;
        S.Active.LastAnchor=Anchor;
        if(FollowBottom>0&&S.Active.Ready)S.Active.LastAnchor.Y=FMath::Min(Anchor.Y,FollowBottom-S.Active.FollowHeight);
        const double Age=UIMotionReviewV83::TooltipAge>=0?UIMotionReviewV83::TooltipAge:Now-S.HoverAt;
        if(Age<.25)return;
        if(!S.Active.Ready||S.Active.Revision!=Revision)
        {
            S.Active.Ops.Reset();if(S.Active.Ops.Max()<192)S.Active.Ops.Reserve(192);
            RecordingV83=&S.Active.Ops;Build();RecordingV83=nullptr;
            if(FollowBottom>0)
            {
                for(const auto& O:S.Active.Ops)if(O.Kind==ETooltipOpV83::Hit){S.Active.FollowHeight=O.H;break;}
                Anchor.Y=FMath::Min(Anchor.Y,FollowBottom-S.Active.FollowHeight);S.Active.LastAnchor=Anchor;
            }
            S.Active.CapturedAnchor=Anchor;S.Active.Revision=Revision;S.Active.Ready=true;
        }
    }
    // Direct skin texture calls also pass here; unlike Image(), signed UV
    // spans are permitted for the existing mirrored nine-slice grain.
    void TextureV83(UTexture* T,float SX,float SY,float SW,float SH,float U,float V,float UW,float VH,FLinearColor Tint)const
    {
        if(RecordingV83){auto& O=OpV83(ETooltipOpV83::Texture,(SX-OX)/Scale,(SY-OY)/Scale,SW/Scale,SH/Scale,Tint);O.Texture=T;O.U=U;O.V=V;O.UW=UW;O.VH=VH;return;}
        HUD.DrawTexture(T,SX+ReplayXV83*Scale,SY+ReplayYV83*Scale,SW,SH,U,V,UW,VH,MotionInkV83(Tint));
    }
    void ReplayTooltipV83(const FTooltipBufferV83& B,float Alpha,float Offset,bool Hits)const
    {
        if(Alpha<=0||!B.Ready)return;
        ReplayAlphaV83=Alpha;ReplayXV83=B.LastAnchor.X-B.CapturedAnchor.X;
        ReplayYV83=Offset+B.LastAnchor.Y-B.CapturedAnchor.Y;
        for(const auto& O:B.Ops)switch(O.Kind)
        {
            case ETooltipOpV83::Rect:Rect(O.X,O.Y,O.W,O.H,O.Ink);break;
            case ETooltipOpV83::Line:Line(O.X,O.Y,O.W,O.H,O.Ink,O.Width);break;
            case ETooltipOpV83::Gradient:Gradient(O.X,O.Y,O.W,O.H,O.Ink,O.End,O.Vertical);break;
            case ETooltipOpV83::Triangle:Triangle({O.X,O.Y},{O.W,O.H},O.Third,O.Ink);break;
            case ETooltipOpV83::Texture:TextureV83(O.Texture,OX+O.X*Scale,OY+O.Y*Scale,O.W*Scale,O.H*Scale,O.U,O.V,O.UW,O.VH,O.Ink);break;
            case ETooltipOpV83::Material:MaterialTile(O.Material,O.X,O.Y,O.W,O.H,O.Coverage,O.U,O.V,O.UW,O.VH,O.Ink);break;
            case ETooltipOpV83::Text:
            {
                if(O.ShapedV8210){DrawShapedV8210(Screen(O.X,O.Y),O.Text.ToString(),O.Font,MotionInkV83(O.Ink),O.W);break;}
                FCanvasTextItem Item(Screen(O.X,O.Y),O.Text,O.Font,MotionInkV83(O.Ink));Item.BlendMode=SE_BLEND_Translucent;Surface->DrawItem(Item);break;
            }
            case ETooltipOpV83::Hit:if(Hits)
            {
                const float X=O.X+ReplayXV83,Y=O.Y+ReplayYV83;
                TGuardValue<float> GX(ReplayXV83,0),GY(ReplayYV83,0);
                Hit(X,Y,O.W,O.H,O.HitName,O.Priority);
            }break;
        }
        ReplayAlphaV83=1;ReplayXV83=ReplayYV83=0;
    }
    void FinishTooltipsV83()const
    {
        for(const auto& Buffer:PageTooltipsV8222)ReplayTooltipV83(Buffer,1.f,0.f,true);
        PageTooltipsV8222.Reset();
        if(!MotionFieldV83||MotionStableV83||!TooltipStateV83)return;
        auto& S=*TooltipStateV83;const double Now=FPlatformTime::Seconds();
        if(S.Candidate!=S.Pending)
        {
            if(S.Active.Ready)
            {
                const auto Sample=FrontlineUIMotionV83::TooltipSample(UIMotionReviewV83::TooltipAge>=0?UIMotionReviewV83::TooltipAge:Now-S.HoverAt);
                Swap(S.Active,S.Leaving);S.Leaving.FadeAt=Now;S.Leaving.FadeAlpha=Sample.Alpha;S.Leaving.FadeOffset=Sample.OffsetY;
            }
            S.Active.Ops.Reset();S.Active.Ready=false;S.Pending=S.Candidate;S.HoverAt=Now;
        }
        if(S.Leaving.Ready)
        {
            const auto Sample=FrontlineUIMotionV83::TooltipSample(.37,Now-S.Leaving.FadeAt,S.Leaving.FadeAlpha);
            ReplayTooltipV83(S.Leaving,Sample.Alpha,S.Leaving.FadeOffset,false);
            if(Sample.Alpha<=0){S.Leaving.Ready=false;S.Leaving.Ops.Reset();}
        }
        if(S.Pending&&S.Active.Ready)
        {
            const auto Sample=FrontlineUIMotionV83::TooltipSample(UIMotionReviewV83::TooltipAge>=0?UIMotionReviewV83::TooltipAge:Now-S.HoverAt);
            ReplayTooltipV83(S.Active,Sample.Alpha,Sample.OffsetY,Sample.Alpha>=1);
        }
    }
