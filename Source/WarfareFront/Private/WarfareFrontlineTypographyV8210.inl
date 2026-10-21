    mutable bool bLegacyTypographyV8210=false;
    mutable bool bTooltipTypographyV8210=false;
    mutable bool bRoleChineseV8210=false;
    mutable bool bResolvedTextV8210=false;
    struct FRoleLayoutKeyV834
    {
        FString Value;FSlateFontInfo Face;
        float Width=0,Height=0,Em=0,OutputScale=0,DPI=0;
        int32 Alignment=0,FontRevision=0;uint8 Role=0,Language=0;
        bool Chinese=false,Tooltip=false,EnglishPrimary=false,Resolved=false;
        bool operator==(const FRoleLayoutKeyV834& B)const
        {
            return Value.Equals(B.Value,ESearchCase::CaseSensitive)&&Face==B.Face&&Face.FontFallback==B.Face.FontFallback
                &&Width==B.Width&&Height==B.Height&&Em==B.Em&&OutputScale==B.OutputScale&&DPI==B.DPI
                &&Alignment==B.Alignment&&FontRevision==B.FontRevision&&Role==B.Role&&Language==B.Language
                &&Chinese==B.Chinese&&Tooltip==B.Tooltip&&EnglishPrimary==B.EnglishPrimary&&Resolved==B.Resolved;
        }
        friend uint32 GetTypeHash(const FRoleLayoutKeyV834& K)
        {
            uint32 Hash=FCrc::MemCrc32(*K.Value,K.Value.Len()*sizeof(TCHAR));
            Hash=HashCombine(Hash,GetTypeHash(K.Face));Hash=HashCombine(Hash,GetTypeHash(uint8(K.Face.FontFallback)));
            for(float V:{K.Width,K.Height,K.Em,K.OutputScale,K.DPI})Hash=HashCombine(Hash,GetTypeHash(V==0.f?0.f:V));
            Hash=HashCombine(Hash,GetTypeHash(K.Alignment));Hash=HashCombine(Hash,GetTypeHash(K.FontRevision));
            Hash=HashCombine(Hash,GetTypeHash(uint32(K.Role)|(uint32(K.Language)<<8)|(uint32(K.Chinese)<<16)
                |(uint32(K.Tooltip)<<17)|(uint32(K.EnglishPrimary)<<18)|(uint32(K.Resolved)<<19)));
            return Hash;
        }
    };
    FRoleLayoutKeyV834 LayoutKeyV834(EFrontlineTextRoleV8210 R,const FString& V,float W,float H,float Em,int32 Alignment)const
    {
        FRoleLayoutKeyV834 K;K.Value=V;K.Face=RoleInfoV8210(R,V,Em);K.Width=W;K.Height=H;K.Em=Em;
        K.OutputScale=Scale;K.DPI=Surface->Canvas->GetDPIScale();K.Alignment=Alignment;K.Role=uint8(R);
        K.Language=uint8(FrontlineLocalizationV35::GetLanguage());K.Chinese=bRoleChineseV8210||ContainsCJKV45(V);
        K.Tooltip=bTooltipTypographyV8210;K.EnglishPrimary=bEnglishPrimaryV45;K.Resolved=bResolvedTextV8210;
#if WITH_EDITORONLY_DATA
        if(const auto* Composite=K.Face.GetCompositeFont())K.FontRevision=Composite->HistoryRevision;
#endif
        return K;
    }
    struct FLabelLayoutV834
    {
        FVector2D Metrics=FVector2D::ZeroVector;float Em=0;FShapedGlyphSequencePtr Dependency;
        bool Current()const{return Dependency.IsValid()&&!Dependency->IsDirty();}
        bool Same(const FLabelLayoutV834& B)const{return Metrics.X==B.Metrics.X&&Metrics.Y==B.Metrics.Y&&Em==B.Em;}
    };
    struct FParagraphLayoutV834
    {
        TArray<FString> Rows;TArray<float> Fill;
        TArray<FShapedGlyphSequencePtr> Dependencies;
        float Em=0,Leading=0,Height=0,ActualWidth=0;bool Truncated=false;
        bool Current()const
        {
            if(Dependencies.IsEmpty())return false;
            for(const auto& Sequence:Dependencies)if(!Sequence.IsValid()||Sequence->IsDirty())return false;
            return true;
        }
        bool Same(const FParagraphLayoutV834& B)const
        {
            if(Em!=B.Em||Leading!=B.Leading||Height!=B.Height||ActualWidth!=B.ActualWidth||Truncated!=B.Truncated
                ||Rows.Num()!=B.Rows.Num()||Fill.Num()!=B.Fill.Num())return false;
            for(int32 I=0;I<Rows.Num();++I)if(!Rows[I].Equals(B.Rows[I],ESearchCase::CaseSensitive))return false;
            for(int32 I=0;I<Fill.Num();++I)if(Fill[I]!=B.Fill[I])return false;
            return true;
        }
    };
    static bool UppercaseV8210(const FString& S)
    {
        bool Letter=false;
        for(TCHAR C:S){if(FChar::IsLower(C))return false;if(FChar::IsUpper(C))Letter=true;}
        return Letter;
    }
    EFrontlineTextRoleV8210 DefaultRoleV8210(const FString& S)const
    {
        if(bTooltipTypographyV8210)return EFrontlineTextRoleV8210::DarkText;
        bool Digit=false,Numeric=true;
        for(TCHAR C:S)
        {
            if(FChar::IsDigit(C)){Digit=true;continue;}
            if(!FChar::IsWhitespace(C)&&C!='.'&&C!=','&&C!=':'&&C!='/'&&C!='%'&&C!='+'&&C!='-'&&C!='('&&C!=')'&&C!=0x00b1&&C!=0x00d7&&C!=0x2212&&C!=0x00b0){Numeric=false;break;}
        }
        if(Digit&&Numeric)return EFrontlineTextRoleV8210::Numeral;
        return UppercaseV8210(S)?EFrontlineTextRoleV8210::Plate:EFrontlineTextRoleV8210::Text;
    }
    float DefaultEmV8210(const FString& S,float Old)const
    {return ContainsCJKV45(S)?FMath::Max(15.f,Old):Old*1.18f;}
    FSlateFontInfo RoleInfoV8210(EFrontlineTextRoleV8210 R,const FString& V,float Em,int32 ExtraTracking=0)const
    {
        const bool CJK=bRoleChineseV8210||ContainsCJKV45(V);auto& A=Artwork();UFont* Face=nullptr;int32 Tracking=0;
        using Rl=EFrontlineTextRoleV8210;
        if(CJK)
        {
            const bool Medium=R==Rl::Plate||R==Rl::Headline||R==Rl::Label||R==Rl::Objective||R==Rl::FieldValue||R==Rl::Inscription;
            Face=R==Rl::Annotation?A.KaiMixedV8210.Get():(R==Rl::Plate||R==Rl::Headline||R==Rl::Label||R==Rl::Inscription)?A.MediumControlMixedV8210.Get():Medium?A.MediumMixedV8210.Get():A.RegularMixedV8210.Get();
            Tracking=R==Rl::Plate?200:R==Rl::Headline?100:R==Rl::Label?240:R==Rl::Objective?60:R==Rl::FieldValue?120:R==Rl::Inscription?500:R==Rl::DarkText?50:40;
        }
        else
        {
            switch(R)
            {
                case Rl::Plate:Face=A.ControlRuntimeV62;Tracking=60;break;
                case Rl::Label:Face=A.ControlSemiBoldV8210;Tracking=200;break;
                case Rl::Headline:Face=A.ControlRuntimeV62;Tracking=10;break;
                case Rl::Inscription:Face=A.ControlSemiBoldV8210;Tracking=260;break;
                case Rl::Typed:Face=A.TypedV8210;Tracking=40;break;
                case Rl::Stencil:Face=A.StencilV8210;Tracking=130;break;
                case Rl::Numeral:Face=A.ControlRuntimeV62;Tracking=20;break;
                case Rl::Objective:Face=A.TextMediumV8210;break;
                case Rl::Annotation:Face=A.TextItalicV8210;break;
                default:Face=A.TextV8210;break;
            }
        }
        if(!Face)Face=CJK?ChineseFont:Font;
        auto Info=Face->GetLegacySlateFontInfo();Info.Size=FMath::Max(1.f,FMath::RoundToFloat(Em*Scale*.75f*2.f)*.5f);
        // Slate quantizes tracking to pixel advances using point size, not em.
        const float Desired=(Tracking+ExtraTracking)*Em*RasterUnitsV825()/1000.f;
        const int32 Pixels=Tracking+ExtraTracking>0?FMath::Max(1,FMath::RoundToInt(Desired)):FMath::RoundToInt(Desired);
        Info.LetterSpacing=Pixels?FMath::CeilToInt(Pixels*1000.f/Info.Size)+1:0;
        if(R==Rl::Numeral&&!CJK){Info.bForceMonospaced=true;Info.MonospacedWidth=.534f*(4.f/3.f);}
        return Info;
    }
    FShapedGlyphSequencePtr ShapedV8210(const FString& V,const FSlateFontInfo& Info,float JustifyWidth=0.f)const
    {
        if(V.IsEmpty())return {};
        const auto FontCache=FEngineFontServices::Get().GetFontCache();if(!FontCache.IsValid())return {};
        const float DPI=Surface->Canvas->GetDPIScale();
        const int32 Target=JustifyWidth>0.f?FMath::FloorToInt(JustifyWidth*RasterUnitsV825()):0;
        static TBoundedTextCacheV8213<FShapedGlyphSequencePtr,4096> Cache;
        const FString Key=FString::Printf(TEXT("%p|%p|%s|%.3f|%d|%.5f|%d|%.3f|%d|%s"),Info.FontObject.Get(),Info.CompositeFont.Get(),*Info.TypefaceFontName.ToString(),Info.Size,Info.LetterSpacing,DPI,Info.bForceMonospaced,Info.MonospacedWidth,Target,*V);
        if(const auto* Found=Cache.Find(Key))if(Found->IsValid()&&!(*Found)->IsDirty())return *Found;
        FShapedGlyphSequencePtr Sequence=FontCache->ShapeUnidirectionalText(V,Info,DPI,TextBiDi::ETextDirection::LeftToRight,ETextShapingMethod::FullShaping);
        if(Info.bForceMonospaced)
        {
            // FullShaping does not implement Slate's pseudo-monospacing. The
            // approved Oswald 500 zero advance is 534/1000 em; centre digits in
            // that cell while retaining the ordinary punctuation advances.
            TArray<FShapedGlyphEntry> Glyphs=Sequence->GetGlyphsToRender();
            const int32 Cell=FMath::RoundToInt(Info.MonospacedWidth*Info.Size*DPI);
            const int32 Tracking=FMath::TruncToInt(Info.LetterSpacing*Info.Size/1000.f);
            for(int32 I=0;I<Glyphs.Num();++I)
            {
                auto& Glyph=Glyphs[I];if(!V.IsValidIndex(Glyph.SourceIndex)||!FChar::IsDigit(V[Glyph.SourceIndex]))continue;
                const int32 Gap=I+1<Glyphs.Num()?Tracking:0;
                Glyph.XOffset+=int16((Cell-(int32(Glyph.XAdvance)-Gap))/2);
                Glyph.XAdvance=int16(FMath::Min(int32(MAX_int16),Cell+Gap));
            }
            Sequence=MakeShared<FShapedGlyphSequence>(MoveTemp(Glyphs),Sequence->GetTextBaseline(),Sequence->GetMaxTextHeight(),
                Sequence->GetFontMaterial(),Sequence->IsMaterialStencil(),Sequence->GetFontOutlineSettings(),FShapedGlyphSequence::FSourceTextRange(0,V.Len()));
        }
        // LetterSpacing has whole-pixel advances. Distribute the remainder over
        // cluster boundaries, then draw the complete shaped row in one batch.
        if(Target>Sequence->GetMeasuredWidth())
        {
            TArray<FShapedGlyphEntry> Glyphs=Sequence->GetGlyphsToRender();TArray<int32> Gaps;
            for(int32 I=0;I+1<Glyphs.Num();++I)if(Glyphs[I].SourceIndex!=Glyphs[I+1].SourceIndex)Gaps.Add(I);
            const int32 Remainder=Target-Sequence->GetMeasuredWidth();
            if(!Gaps.IsEmpty())
            {
                for(int32 I=0;I<Gaps.Num();++I)
                {
                    const int32 Extra=((I+1)*Remainder)/Gaps.Num()-(I*Remainder)/Gaps.Num();
                    auto& Glyph=Glyphs[Gaps[I]];Glyph.XAdvance=int16(FMath::Min(int32(MAX_int16),int32(Glyph.XAdvance)+Extra));
                }
                Sequence=MakeShared<FShapedGlyphSequence>(MoveTemp(Glyphs),Sequence->GetTextBaseline(),Sequence->GetMaxTextHeight(),
                    Sequence->GetFontMaterial(),Sequence->IsMaterialStencil(),Sequence->GetFontOutlineSettings(),FShapedGlyphSequence::FSourceTextRange(0,V.Len()));
            }
        }
        Cache.Add(Key,Sequence);return Sequence;
    }
    void DrawShapedV8210(const FVector2D& Position,const FString& V,const FSlateFontInfo& Info,const FLinearColor& Ink,float JustifyWidth=0.f)const
    {
        const auto Sequence=ShapedV8210(V,Info,JustifyWidth);if(!Sequence.IsValid())return;
        const float RawDPI=Surface->Canvas->GetDPIScale();
        const float DPI=FMath::IsFinite(RawDPI)&&RawDPI>0.f?RawDPI:1.f;
        // Engine CanvasTextItem cancels its DPI in the glyph batch transform;
        // CanvasShapedTextItem does not. Shape at DPI for crisp atlas glyphs,
        // then normalize BOTH the anchor and glyph geometry to panel pixels.
        // Output resolution is already in Info.Size; it must not be applied again.
        FMeasuredCanvasShapedTextV8212 Item(Position/DPI,Sequence.ToSharedRef(),Ink);
        Item.Scale=FVector2D(1.f/DPI,1.f/DPI);
        Item.BlendMode=SE_BLEND_Translucent;Surface->DrawItem(Item);
        if(V825FontAuditActive||(V45HUDAuditActive&&V66HUDModernAudit))
        {
            const FString Key=FString::Printf(TEXT("%p|%.3f|%.5f|%.5f|%.3f,%.3f|%s"),Info.FontObject.Get(),Info.Size,Scale,DPI,Position.X,Position.Y,*V);
            const bool ModernChinese=V45HUDAuditActive&&V66HUDModernAudit&&ContainsCJKV45(V);
            // A fading previous tooltip is not evidence for the next hover.
            // Only sample visible ink; EndAudit also requires new text at hover 2.
            const bool VisibleHover=ModernChinese&&V66HUDHoverIndex>0&&Ink.A>.5f;
            const FString HoverKey=VisibleHover?FString::Printf(TEXT("%d|%p|%.3f|%s"),V66HUDHoverIndex,Info.FontObject.Get(),Info.Size,*V):FString();
            if(!V8212ShapedFontDraws.Contains(Key)||(VisibleHover&&!V45ChineseDraws.Contains(HoverKey)))
            {
                // Actual DrawnSize is from the engine glyph loop after Item.Scale.
                // The shaped sequence is still in raster pixels, hence /DPI only
                // on the independently measured side of this comparison.
                const FVector2D Drawn=Item.DrawnSize/FMath::Max(.001f,Scale);
                const FVector2D Measured=FVector2D(Sequence->GetMeasuredWidth(),Sequence->GetMaxTextHeight())/RasterUnitsV825();
                auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("text"),V);J->SetStringField(TEXT("font"),Info.FontObject.Get()?Info.FontObject->GetPathName():FString());
                J->SetNumberField(TEXT("raster_point_size"),Info.Size);J->SetNumberField(TEXT("canvas_dpi"),DPI);J->SetNumberField(TEXT("output_scale"),Scale);
                J->SetBoolField(TEXT("hdr_sdr_composite"),bHDRComposite);
                J->SetNumberField(TEXT("actual_draw_width"),Drawn.X);J->SetNumberField(TEXT("actual_draw_height"),Drawn.Y);
                J->SetNumberField(TEXT("measure_width"),Measured.X);J->SetNumberField(TEXT("measure_height"),Measured.Y);
                J->SetNumberField(TEXT("width_error"),FMath::Abs(Drawn.X-Measured.X));J->SetNumberField(TEXT("height_error"),FMath::Abs(Drawn.Y-Measured.Y));
                J->SetNumberField(TEXT("expected_anchor_x"),Position.X);J->SetNumberField(TEXT("expected_anchor_y"),Position.Y);
                J->SetNumberField(TEXT("actual_anchor_x"),Item.PaintedOrigin.X);J->SetNumberField(TEXT("actual_anchor_y"),Item.PaintedOrigin.Y);
                J->SetNumberField(TEXT("anchor_error"),(Item.PaintedOrigin-Position).GetAbsMax());
                if(ModernChinese)
                {
                    const auto FontCache=FEngineFontServices::Get().GetFontCache();auto& A=Artwork();
                    const UObject* Face=Info.FontObject.Get();FString Family;
                    if(Face&&Face==A.RegularMixedV8210.Get())Family=TEXT("RegularMixedV8210");
                    else if(Face&&Face==A.MediumMixedV8210.Get())Family=TEXT("MediumMixedV8210");
                    else if(Face&&Face==A.MediumControlMixedV8210.Get())Family=TEXT("MediumControlMixedV8210");
                    else if(Face&&Face==A.KaiMixedV8210.Get())Family=TEXT("KaiMixedV8210");
                    bool Sources=true,Glyphs=FontCache.IsValid(),Atlases=FontCache.IsValid(),PackagedGlyphs=FontCache.IsValid();
                    int32 VisibleGlyphs=0,ChineseCharacters=0;TSet<int32> ChineseSources;TSet<FString> Assets;
                    for(const auto& Glyph:Sequence->GetGlyphsToRender())if(Glyph.bIsVisible)
                    {
                        ++VisibleGlyphs;const int32 Count=FMath::Max(1,int32(Glyph.NumCharactersInGlyph));
                        const bool SourceValid=V.IsValidIndex(Glyph.SourceIndex)&&Glyph.SourceIndex+Count<=V.Len();
                        Sources&=SourceValid;Glyphs&=Glyph.HasValidGlyph();
                        if(FontCache.IsValid())Atlases&=FontCache->GetShapedGlyphFontAtlasData(Glyph,FFontOutlineSettings::NoOutline).Valid;
                        if(SourceValid)for(int32 I=Glyph.SourceIndex;I<Glyph.SourceIndex+Count;++I)
                            if(V[I]>=0x4e00&&V[I]<=0x9fff)ChineseSources.Add(I);
                    }
                    for(int32 I=0;I<V.Len();++I)if(V[I]>=0x4e00&&V[I]<=0x9fff)
                    {
                        ++ChineseCharacters;
                        if(FontCache.IsValid())
                        {
                            float FontScale=1.f;const auto& Data=FontCache->GetFontDataForCodepoint(Info,UTF32CHAR(V[I]),FontScale);
                            PackagedGlyphs&=FontCache->CanLoadCodepoint(Data,UTF32CHAR(V[I]),EFontFallback::FF_NoFallback);
                            if(const UObject* Asset=Data.GetFontFaceAsset())Assets.Add(Asset->GetPathName());
                        }
                    }
                    const bool Finite=FMath::IsFinite(RawDPI)&&RawDPI>0&&FMath::IsFinite(Scale)&&Scale>0
                        &&FMath::IsFinite(Drawn.X)&&FMath::IsFinite(Drawn.Y)&&FMath::IsFinite(Measured.X)&&FMath::IsFinite(Measured.Y)
                        &&FMath::IsFinite(Position.X)&&FMath::IsFinite(Position.Y)&&FMath::IsFinite(Item.PaintedOrigin.X)&&FMath::IsFinite(Item.PaintedOrigin.Y);
                    J->SetBoolField(TEXT("actual_shaped_glyph_loop"),Drawn.X>0&&Drawn.Y>0&&VisibleGlyphs>0);
                    J->SetBoolField(TEXT("bounds_finite"),Finite);J->SetBoolField(TEXT("modern_cjk_face_family"),!Family.IsEmpty());
                    J->SetStringField(TEXT("approved_face_family"),Family);J->SetNumberField(TEXT("hover_index"),V66HUDHoverIndex);
                    J->SetNumberField(TEXT("ink_alpha"),Ink.A);J->SetNumberField(TEXT("frame"),double(GFrameCounter));
                    J->SetNumberField(TEXT("visible_glyphs"),VisibleGlyphs);J->SetNumberField(TEXT("chinese_characters"),ChineseCharacters);
                    J->SetBoolField(TEXT("glyph_source_indices_valid"),Sources);J->SetBoolField(TEXT("visible_glyphs_valid"),Glyphs);
                    J->SetBoolField(TEXT("glyph_atlas_entries_valid"),Atlases);
                    J->SetBoolField(TEXT("cjk_source_coverage"),ChineseCharacters>0&&ChineseSources.Num()==ChineseCharacters);
                    J->SetBoolField(TEXT("cjk_face_has_glyphs_without_fallback"),PackagedGlyphs);
                    J->SetBoolField(TEXT("closing_punctuation_at_row_start"),!V.IsEmpty()&&ClosingPunctuationV45(V[0]));
                    TArray<FString> AssetNames=Assets.Array();AssetNames.Sort();TArray<TSharedPtr<FJsonValue>> FontAssets;
                    for(const auto& Name:AssetNames)FontAssets.Add(MakeShared<FJsonValueString>(Name));J->SetArrayField(TEXT("selected_font_face_assets"),FontAssets);
                    if(VisibleHover&&!V45ChineseDraws.Contains(HoverKey))V45ChineseDraws.Add(HoverKey,J);
                }
                if(!V8212ShapedFontDraws.Contains(Key))V8212ShapedFontDraws.Add(Key,J);
            }
        }
    }
    FString RoleValueV8210(EFrontlineTextRoleV8210 R,const FString& Source)const
    {
        using Rl=EFrontlineTextRoleV8210;
        FString V=bResolvedTextV8210?Source:(R==Rl::Typed||R==Rl::Stencil)?FrontlineLocalizationV35::English(Source):DisplayText(Source);
        if(!ContainsCJKV45(V)&&(R==Rl::Plate||R==Rl::Label||R==Rl::Inscription||R==Rl::Typed||R==Rl::Stencil))V.ToUpperInline();
        return V;
    }
    FVector2D RoleSizeRawV8210(EFrontlineTextRoleV8210 R,const FString& V,float Em,int32 Extra=0,float JustifyWidth=0.f)const
    {
        if(V.IsEmpty())return FVector2D::ZeroVector;
        auto Info=RoleInfoV8210(R,V,Em,Extra);
        const auto Sequence=ShapedV8210(V,Info,JustifyWidth);if(!Sequence.IsValid())return FVector2D::ZeroVector;
        FVector2D Size=FVector2D(Sequence->GetMeasuredWidth(),Sequence->GetMaxTextHeight())/RasterUnitsV825();
        const bool Caps=Info.FontObject.Get()==Artwork().ControlRuntimeV62.Get()||Info.FontObject.Get()==Artwork().ControlSemiBoldV8210.Get();
        if(Caps){Size.Y=Info.Size*(4.f/3.f)*.810f/FMath::Max(.001f,Scale);
            bool Descender=false;for(TCHAR C:V)if(C=='g'||C=='j'||C=='p'||C=='q'||C=='y'||C==0x0443||C==0x0444||C==0x0434||C==0x0446||C==0x0449||C==0x0414||C==0x0426||C==0x0429){Descender=true;break;}
            if(Descender)Size.Y+=Em*.20f;}
        return Size;
    }
    float RoleBaselineRawV8210(EFrontlineTextRoleV8210 R,const FString& V,float Em)const
    {
        const auto Info=RoleInfoV8210(R,V,Em);const auto Sequence=ShapedV8210(V,Info);
        if(!Sequence.IsValid())return 0.f;
        const bool Caps=Info.FontObject.Get()==Artwork().ControlRuntimeV62.Get()||Info.FontObject.Get()==Artwork().ControlSemiBoldV8210.Get();
        const float Inset=Caps?Info.Size*(4.f/3.f)*.383f/FMath::Max(.001f,Scale):0.f;
        return (Sequence->GetMaxTextHeight()+Sequence->GetTextBaseline())/RasterUnitsV825()-Inset;
    }
    void RoleDrawRawV8210(EFrontlineTextRoleV8210 R,const FString& V,float X,float Y,float Em,FLinearColor Ink,int32 Extra=0,float JustifyWidth=0.f)const
    {
        if(V.IsEmpty()||Em<=0)return;
        const auto Info=RoleInfoV8210(R,V,Em,Extra);
        const bool Caps=Info.FontObject.Get()==Artwork().ControlRuntimeV62.Get()||Info.FontObject.Get()==Artwork().ControlSemiBoldV8210.Get();
        const float Inset=Caps?Info.Size*(4.f/3.f)*.383f:0.f;
        const FVector2D P(FMath::RoundToFloat(OX+X*Scale),FMath::RoundToFloat(OY+Y*Scale-Inset));
        if(RecordingV83){auto& O=OpV83(ETooltipOpV83::Text,(P.X-OX)/Scale,(P.Y-OY)/Scale,JustifyWidth,0,Ink);O.Text=FText::FromString(V);O.Font=Info;O.ShapedV8210=true;return;}
        DrawShapedV8210(P+FVector2D(ReplayXV83*Scale,ReplayYV83*Scale),V,Info,MotionInkV83(Ink),JustifyWidth);
    }
    float RoleWidth(EFrontlineTextRoleV8210 R,const FString& Source,float Em)const
    {return RoleSizeRawV8210(R,RoleValueV8210(R,Source),Em).X;}
    void RoleText(EFrontlineTextRoleV8210 R,const FString& Source,float X,float Y,float Em,const FLinearColor& Ink=Paper)const
    {RoleDrawRawV8210(R,RoleValueV8210(R,Source),X,Y,Em,Ink);}
    void RoleLabel(EFrontlineTextRoleV8210 R,const FString& Source,float X,float Y,float W,float H,float Em,const FLinearColor& Ink=Paper,int32 Align=0)const
    {
        const FString V=RoleValueV8210(R,Source);if(V.IsEmpty()||W<=0||H<=0)return;
        static TLayoutCacheV834<FRoleLayoutKeyV834,FLabelLayoutV834,512> Cache;
        static TSet<FRoleLayoutKeyV834> Reviewed;
        const auto Key=LayoutKeyV834(R,V,W,H,Em,Align);
        auto Build=[&]()
        {
            FLabelLayoutV834 Layout;Layout.Em=Em;Layout.Metrics=RoleSizeRawV8210(R,V,Layout.Em);
            const float Minimum=FMath::Min(Layout.Em,ContainsCJKV45(V)?13.f:12.f);
            while((Layout.Metrics.X>W+.1f||Layout.Metrics.Y>H+.1f)&&Layout.Em>Minimum)
            {Layout.Em=FMath::Max(Minimum,Layout.Em-.5f);Layout.Metrics=RoleSizeRawV8210(R,V,Layout.Em);}
            Layout.Dependency=ShapedV8210(V,RoleInfoV8210(R,V,Layout.Em));return Layout;
        };
        const auto* Cached=FrontlineTextLayoutV834::CacheEnabled()?Cache.Find(Key):nullptr;
        FLabelLayoutV834 Fresh;const FLabelLayoutV834* Layout=Cached;
        auto& Counts=FrontlineTextLayoutV834::ReviewState();
        if(!Cached||!Cached->Current())
        {Fresh=Build();Layout=&Fresh;++Counts.LabelMisses;if(FrontlineTextLayoutV834::CacheEnabled()&&Fresh.Current())Cache.Add(Key,Fresh);}
        else
        {
            ++Counts.LabelHits;
            if(FrontlineTextLayoutV834::ReviewEnabled()&&Reviewed.Num()<1024&&!Reviewed.Contains(Key))
            {
                Fresh=Build();++Counts.ParityChecks;Reviewed.Add(Key);
                if(!Cached->Same(Fresh)){++Counts.ParityFailures;Layout=&Fresh;Cache.Add(Key,Fresh);}
            }
        }
        Counts.LabelEntries=Cache.Entries.Num();Em=Layout->Em;const auto& M=Layout->Metrics;
        const float TX=X+(Align==1?(W-M.X)*.5f:Align==2?W-M.X:0),TY=Y+(H-M.Y)*.5f;
        RoleDrawRawV8210(R,V,TX,TY,Em,Ink);
        if(FrontlineUIAssetsV8213::BaselineMode()||V36TypographyAuditActive)RecordTypographyV36(V,X,Y,W,H,M.X,M.Y,Em,false,Align,TX,TY,RoleInfoV8210(R,V,Em).FontObject->GetPathName());
    }
    static bool OpeningV8210(TCHAR C)
    {return C==0x201c||C==0x2018||C==0xff08||C==0xff3b||C==0xff5b||C==0x3008||C==0x300a||C==0x300c||C==0x300e||C==0x3010||C==0x3014||C=='('||C=='['||C=='{';}
    TArray<FString> RoleRowsV8210(EFrontlineTextRoleV8210 R,const FString& V,float W,float Em,int32 Extra=0)const
    {
        if(V.IsEmpty()||W<=0.f||Em<=0.f)return {};
        TGuardValue<bool> FaceScope(bRoleChineseV8210,bRoleChineseV8210||ContainsCJKV45(V));
        static TBoundedTextCacheV8213<TArray<FString>,512> Cache;
        const auto Info=RoleInfoV8210(R,V,Em,Extra);
        const FString Key=FString::Printf(TEXT("%p|%d|%.2f|%.3f|%.5f|%.3f|%s"),Info.FontObject.Get(),Info.LetterSpacing,Info.Size,W,Scale,Surface->Canvas->GetDPIScale(),*V);
        if(const auto* Found=Cache.Find(Key))return *Found;
        TArray<FString> Rows,Blocks;V.ParseIntoArray(Blocks,TEXT("\n"),false);
        for(const auto& Block:Blocks)
        {
            const int32 BlockStart=Rows.Num();
            if(Block.IsEmpty()){Rows.Add(TEXT(""));continue;}
            TArray<int32> Boundaries;auto It=FBreakIterator::CreateLineBreakIterator();It->SetStringRef(Block);
            It->ResetToBeginning();for(int32 E=It->MoveToNext();E!=INDEX_NONE;E=It->MoveToNext())Boundaries.Add(E);
            int32 Begin=0,Last=0,Preferred=INDEX_NONE;
            for(int32 End:Boundaries)
            {
                if(End<Block.Len()&&(ClosingPunctuationV45(Block[End])||OpeningV8210(Block[End-1])))continue;
                if(End>Begin&&ClosingPunctuationV45(Block[End-1]))Preferred=End;
                if(RoleSizeRawV8210(R,Block.Mid(Begin,End-Begin).TrimEnd(),Em,Extra).X>W&&Last>Begin)
                {
                    int32 Cut=Last;
                    if((R==EFrontlineTextRoleV8210::Annotation||R==EFrontlineTextRoleV8210::DarkText)&&Preferred>Begin&&Preferred<=Last
                        &&RoleSizeRawV8210(R,Block.Mid(Begin,Preferred-Begin),Em,Extra).X>W*.6f)Cut=Preferred;
                    Rows.Add(Block.Mid(Begin,Cut-Begin).TrimStartAndEnd());Begin=Cut;Preferred=INDEX_NONE;
                }
                Last=End;
            }
            Rows.Add(Block.Mid(Begin).TrimStartAndEnd());
            if(ContainsCJKV45(Block)&&Rows.Num()>BlockStart+1&&!Rows.Last().IsEmpty()&&Rows.Last().Len()<4)
            {
                auto& Prev=Rows[Rows.Num()-2];auto& Tail=Rows.Last();FString Borrowed=Prev,Result=Tail;
                while((Result.Len()<4||ClosingPunctuationV45(Result[0]))&&Borrowed.Len()>4
                    &&(ContainsCJKV45(Borrowed.Right(1))||ClosingPunctuationV45(Borrowed[Borrowed.Len()-1])))
                {
                    Result=Borrowed.Right(1)+Result;Borrowed.LeftChopInline(1);
                    if(!Borrowed.IsEmpty()&&OpeningV8210(Borrowed[Borrowed.Len()-1])){Result=Borrowed.Right(1)+Result;Borrowed.LeftChopInline(1);}
                }
                if(Result.Len()>=4&&!ClosingPunctuationV45(Result[0])&&RoleSizeRawV8210(R,Result,Em,Extra).X<=W+.1f){Prev=MoveTemp(Borrowed);Tail=MoveTemp(Result);}
            }
        }
        Cache.Add(Key,Rows);return Rows;
    }
    float RoleParagraph(EFrontlineTextRoleV8210 R,const FString& Source,float X,float Y,float W,float H,float Em,const FLinearColor& Ink=Muted)const
    {
        const FString V=RoleValueV8210(R,Source);if(V.IsEmpty()||W<=0||H<=0)return Y;
        const bool CJK=ContainsCJKV45(V);TGuardValue<bool> FaceScope(bRoleChineseV8210,CJK);using Rl=EFrontlineTextRoleV8210;
        if(CJK)Em=FMath::Max(Em,R==Rl::Annotation?19.f:15.f);
        static TLayoutCacheV834<FRoleLayoutKeyV834,FParagraphLayoutV834,512> Cache;
        static TSet<FRoleLayoutKeyV834> Reviewed;
        const auto Key=LayoutKeyV834(R,V,W,H,Em,-1);
        auto Build=[&]()
        {
            FParagraphLayoutV834 Layout;float FitEm=Em,Leading=0,Height=0,Width=W;
            const float Minimum=CJK?(R==Rl::Annotation?19.f:15.f):12.f;
            auto& Rows=Layout.Rows;
            for(;;)
            {
                // These are the unchanged V8210 fit and whole-character rules.
                Width=W;
                if((CJK&&R==Rl::Text)||FrontlineUIAssetsV8213::BaselineMode())
                {
                    const float Cell=FMath::Max(.001f,float(RoleSizeRawV8210(R,TEXT("国国"),FitEm).X-RoleSizeRawV8210(R,TEXT("国"),FitEm).X));
                    if(CJK&&R==Rl::Text)Width=FMath::Max(Cell,FMath::FloorToFloat(W/Cell)*Cell);
                }
                Rows=RoleRowsV8210(R,V,Width,FitEm);
                Leading=FitEm*(CJK?(R==Rl::Annotation?1.8f:R==Rl::Objective?1.9f:R==Rl::DarkText?1.75f:Rows.Num()<3?1.6f:1.85f):(R==Rl::Objective?1.7f:1.48f));
                Height=0;for(const auto& Row:Rows)Height=FMath::Max(Height,float(RoleSizeRawV8210(R,Row,FitEm).Y));
                if((Rows.Num()-1)*Leading+Height<=H+.1f||FitEm<=Minimum)break;
                FitEm=FMath::Max(Minimum,FitEm-.5f);
            }
            const int32 Capacity=FMath::Max(1,1+FMath::FloorToInt((H-Height)/FMath::Max(1.f,Leading)));
            Layout.Truncated=Rows.Num()>Capacity;
            if(Layout.Truncated){Rows.SetNum(Capacity);auto& Tail=Rows.Last();while(!Tail.IsEmpty()&&RoleSizeRawV8210(R,Tail+TEXT("…"),FitEm).X>Width)Tail.LeftChopInline(1);Tail+=TEXT("…");}
            const bool Justify=CJK&&R==Rl::Text&&Rows.Num()>=3&&!bTooltipTypographyV8210;
            TSet<int32> BlockEnds;
            if(Justify){TArray<FString> Blocks;V.ParseIntoArray(Blocks,TEXT("\n"),false);int32 End=0;
                for(const auto& Block:Blocks){End+=Block.IsEmpty()?1:RoleRowsV8210(R,Block,Width,FitEm).Num();BlockEnds.Add(End-1);}}
            Layout.Fill.Reserve(Rows.Num());Layout.Dependencies.Reserve(Rows.Num());
            for(int32 I=0;I<Rows.Num();++I)
            {
                const auto& Row=Rows[I];
                const float Fill=Justify&&I<Rows.Num()-1&&!BlockEnds.Contains(I)&&!Rows[I+1].IsEmpty()&&Row.Len()>1?Width:0.f;
                Layout.Fill.Add(Fill);if(Row.IsEmpty())continue;
                Layout.ActualWidth=FMath::Max(Layout.ActualWidth,float(RoleSizeRawV8210(R,Row,FitEm,0,Fill).X));
                Layout.Dependencies.Add(ShapedV8210(Row,RoleInfoV8210(R,Row,FitEm),Fill));
            }
            Layout.Em=FitEm;Layout.Leading=Leading;Layout.Height=Height;return Layout;
        };
        const auto* Cached=FrontlineTextLayoutV834::CacheEnabled()?Cache.Find(Key):nullptr;
        FParagraphLayoutV834 Fresh;const FParagraphLayoutV834* Layout=Cached;
        auto& Counts=FrontlineTextLayoutV834::ReviewState();
        if(!Cached||!Cached->Current())
        {Fresh=Build();Layout=&Fresh;++Counts.ParagraphMisses;if(FrontlineTextLayoutV834::CacheEnabled()&&Fresh.Current())Cache.Add(Key,Fresh);}
        else
        {
            ++Counts.ParagraphHits;
            if(FrontlineTextLayoutV834::ReviewEnabled()&&Reviewed.Num()<1024&&!Reviewed.Contains(Key))
            {
                Fresh=Build();++Counts.ParityChecks;Reviewed.Add(Key);
                if(!Cached->Same(Fresh)){++Counts.ParityFailures;Layout=&Fresh;Cache.Add(Key,Fresh);}
            }
        }
        Counts.ParagraphEntries=Cache.Entries.Num();Em=Layout->Em;
        const auto& Rows=Layout->Rows;const float Leading=Layout->Leading,Height=Layout->Height;float CY=Y;
        for(int32 I=0;I<Rows.Num();++I)
        {
            const auto& Row=Rows[I];if(Row.IsEmpty()){CY+=Leading*.5f;continue;}
            RoleDrawRawV8210(R,Row,X,CY,Em,Ink,0,Layout->Fill[I]);CY+=Leading;
        }
        // Preserve the old arithmetic from the current absolute origin, including
        // empty-row half leading. Caching a zero-origin height changes rounding.
        const float ActualH=FMath::Max(0.f,CY-Y-Leading+Height);
        if(FrontlineUIAssetsV8213::BaselineMode()||V36TypographyAuditActive)RecordTypographyV36(V,X,Y,W,H,Layout->ActualWidth,ActualH,Em,Layout->Truncated,-1,0,0,RoleInfoV8210(R,V,Em).FontObject->GetPathName(),&Rows);
        return Y+ActualH;
    }
    void BilingualLabel(const FString& EnglishSource,const FString& ChineseSource,float X,float Y,float W,float H,float Em,const FLinearColor& Ink=Paper,int32 Align=0,EFrontlineTextRoleV8210 R=EFrontlineTextRoleV8210::Label)const
    {
        using Rl=EFrontlineTextRoleV8210;
        const FString EN=EnglishSource,ZH=ChineseSource;
        if(bEnglishPrimaryV45||FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::English||ZH==EN||!ContainsCJKV45(ZH))
        {TGuardValue<bool> G(bEnglishPrimaryV45,true);RoleLabel(R,EN,X,Y,W,H,Em,Ink,Align);return;}
        if(Scale<.75f&&R!=Rl::Inscription){RoleLabel(Rl::FieldValue,ZH,X,Y,W,H,FMath::Max(15.f,Em*1.08f),Ink,Align);return;}
        // One hierarchy throughout the Chinese interface; the HUD's explicit
        // English-primary scope retains its original painted legends.
        float ZE=Em,EE=FMath::Max(12.f,Em*(R==Rl::Inscription?.40f:.58f));
        const FString E=R==Rl::Inscription?EN.ToUpper():EN;
        auto ZW=[&](){return RoleSizeRawV8210(R,ZH,ZE).X;};
        auto EW=[&](){return RoleSizeRawV8210(R,E,EE).X;};
        float Gap=Em*.40f;
        for(int32 I=0;I<40&&(ZW()+EW()+Gap>W)&&ZE>15.f;++I){ZE-=.5f;EE=FMath::Max(12.f,ZE*(R==Rl::Inscription?.40f:.58f));Gap=ZE*.40f;}
        const auto ZM=RoleSizeRawV8210(R,ZH,ZE),EM=RoleSizeRawV8210(R,E,EE);
        const float Total=ZM.X+EM.X+Gap,TX=X+(Align==1?(W-Total)*.5f:Align==2?W-Total:0.f),ZY=Y+(H-ZM.Y)*.5f;
        RoleDrawRawV8210(R,ZH,TX,ZY,ZE,Ink);
        auto Faded=Ink;Faded.A*=.68f;
        const float EY=ZY+RoleBaselineRawV8210(R,ZH,ZE)-RoleBaselineRawV8210(R,E,EE);
        RoleDrawRawV8210(R,E,TX+ZM.X+Gap,EY,EE,Faded);
    }
    void BilingualLabel(const FString& Source,float X,float Y,float W,float H,float Em,const FLinearColor& Ink=Paper,int32 Align=0,EFrontlineTextRoleV8210 R=EFrontlineTextRoleV8210::Label)const
    {BilingualLabel(FrontlineLocalizationV35::English(Source),DisplayText(Source),X,Y,W,H,Em,Ink,Align,R);}
    void BilingualTitle(const FString& EnglishSource,const FString& ChineseSource,float X,float Y,float W,float H,float Em,const FLinearColor& Ink=Paper,int32 Align=0)const
    {
        using Rl=EFrontlineTextRoleV8210;
        const FString EN=EnglishSource,ZH=ChineseSource;
        const FString Z=bEnglishPrimaryV45||FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::English?EN:ZH;
        TGuardValue<bool> Resolved(bResolvedTextV8210,true);
        if(!ContainsCJKV45(Z)||Z==EN||Scale<.75f){RoleLabel(Rl::Headline,Z,X,Y,W,H,Em,Ink,Align);return;}
        const float Caption=FMath::Max(12.f,Em*.42f),CH=Caption*.81f;
        RoleLabel(Rl::Headline,Z,X,Y,W,H-CH-5.f,Em,Ink,Align);
        auto Faded=Ink;Faded.A*=.56f;
        const FString V=EN.ToUpper();auto M=RoleSizeRawV8210(Rl::Headline,V,Caption,250);
        float C=Caption;while(M.X>W&&C>12.f){C-=.5f;M=RoleSizeRawV8210(Rl::Headline,V,C,250);}
        RoleDrawRawV8210(Rl::Headline,V,X+(Align==1?(W-M.X)*.5f:Align==2?W-M.X:0),Y+H-CH,C,Faded,250);
    }
    void BilingualTitle(const FString& Source,float X,float Y,float W,float H,float Em,const FLinearColor& Ink=Paper,int32 Align=0)const
    {BilingualTitle(FrontlineLocalizationV35::English(Source),DisplayText(Source),X,Y,W,H,Em,Ink,Align);}
    void ReportStatLabel(const FString& Source,float CX,float Y,float NumberEm,const FLinearColor& Ink=Paper)const
    {
        using Rl=EFrontlineTextRoleV8210;const FString V=DisplayText(Source);
        if(!ContainsCJKV45(V)){RoleLabel(Rl::Label,Source,CX-120,Y,240,35,NumberEm*.6f,Ink,1);return;}
        if(Scale<.75f){RoleLabel(Rl::FieldValue,Source,CX-120,Y,240,35,NumberEm*.6f,Ink,1);return;}
        const float ZE=NumberEm*.6f;const auto Z=RoleSizeRawV8210(Rl::FieldValue,V,ZE,480);
        RoleDrawRawV8210(Rl::FieldValue,V,CX-Z.X*.5f,Y+(35-Z.Y)*.5f,ZE,Ink,480);
    }
    float Description(const FString& Source,float X,float Y,float W,float H,float Em,const FLinearColor& Ink=Muted,bool Light=true)const
    {
        const FString V=DisplayText(Source);
        struct FDescriptionLayersV841 {const TCHAR* Key;const TCHAR* Scene;const TCHAR* Use;const TCHAR* Stats;};
        static const FDescriptionLayersV841 Layers[]={
#include "WarfareFrontlineDescriptionV841.generated.inl"
        };
        for(const auto& Row:Layers)if(V.Equals(Row.Key,ESearchCase::CaseSensitive))
        {
            TGuardValue<bool> Resolved(bResolvedTextV8210,true);float CY=Y;
            auto Draw=[&](EFrontlineTextRoleV8210 Role,const TCHAR* Copy,float Gap)
            {if(!Copy||!*Copy)return;CY=RoleParagraph(Role,Copy,X,CY,W,FMath::Max(1.f,Y+H-CY),Em,Ink)+Gap;};
            Draw(Light?EFrontlineTextRoleV8210::Annotation:EFrontlineTextRoleV8210::Text,Row.Scene,Em*.45f);
            Draw(EFrontlineTextRoleV8210::Text,Row.Use,Em*.35f);
            Draw(EFrontlineTextRoleV8210::Objective,Row.Stats,0.f);
            return CY;
        }
        const int32 Split=V.Find(TEXT("\n"));
        if(Split==INDEX_NONE)return RoleParagraph(EFrontlineTextRoleV8210::Text,Source,X,Y,W,H,Em,Ink);
        const FString Context=V.Left(Split),Effect=V.Mid(Split+1);TGuardValue<bool> Resolved(bResolvedTextV8210,true);
        const float CY=RoleParagraph(Light?EFrontlineTextRoleV8210::Annotation:EFrontlineTextRoleV8210::Text,Context,X,Y,W,H,Em,Ink)+Em*.5f;
        return RoleParagraph(EFrontlineTextRoleV8210::Objective,Effect,X,CY,W,FMath::Max(1.f,Y+H-CY),Em,Ink);
    }

    void ArmyNameV8210(const FString& EN,float X,float Y,float W,float H,float Em,const FLinearColor& Ink=Paper,int32 Align=1,bool German=false)const
    {
        const FString V=EN.ToUpper();TGuardValue<bool> English(bEnglishPrimaryV45,true);
        FString Native=EN;
        if(German){Native=EN.ToLower();for(int32 I=0;I<Native.Len();++I)if(I==0||FChar::IsWhitespace(Native[I-1]))Native[I]=FChar::ToUpper(Native[I]);}
        if(German&&Em>=24.f&&Artwork().BlackletterV8210)
        {
            auto Info=Artwork().BlackletterV8210->GetLegacySlateFontInfo();Info.Size=Em*Scale*.75f;Info.LetterSpacing=0;
            const auto Sequence=ShapedV8210(Native,Info);
            if(Sequence.IsValid())
            {
                const auto Size=FVector2D(Sequence->GetMeasuredWidth(),Sequence->GetMaxTextHeight())/RasterUnitsV825();
                const float Ratio=FMath::Min(1.f,FMath::Min(W/FMath::Max(1.f,float(Size.X)),H/FMath::Max(1.f,float(Size.Y))));
                if(Em*Ratio>=24.f)
                {
                    Info.Size*=Ratio;const auto Fitted=ShapedV8210(Native,Info);
                    const auto M=FVector2D(Fitted->GetMeasuredWidth(),Fitted->GetMaxTextHeight())/RasterUnitsV825();
                    const float TX=X+(Align==1?(W-M.X)*.5f:Align==2?W-M.X:0.f),TY=Y+(H-M.Y)*.5f;
                    DrawShapedV8210(Screen(TX,TY),Native,Info,MotionInkV83(Ink));
                    if(FrontlineUIAssetsV8213::BaselineMode()||V36TypographyAuditActive)RecordTypographyV36(Native,X,Y,W,H,M.X,M.Y,Em*Ratio,false,Align,TX,TY,Info.FontObject->GetPathName());return;
                }
            }
        }
        // Keep the native name and its existing two-line panel rhythm.
        const auto R=EFrontlineTextRoleV8210::Headline;
        TArray<FString> Rows=RoleRowsV8210(R,V,W,Em,230);
        while(Rows.Num()*Em*1.1f>H&&Em>24.f){Em-=.5f;Rows=RoleRowsV8210(R,V,W,Em,230);}
        const float CY=Y+(H-Rows.Num()*Em*1.1f)*.5f;float ActualW=0.f,ActualH=0.f;
        for(int32 I=0;I<Rows.Num();++I){auto M=RoleSizeRawV8210(R,Rows[I],Em,230);float E=Em;
            while(M.X>W&&E>20.f){E-=.5f;M=RoleSizeRawV8210(R,Rows[I],E,230);}
            RoleDrawRawV8210(R,Rows[I],X+(Align==1?(W-M.X)*.5f:Align==2?W-M.X:0.f),CY+I*Em*1.1f,E,Ink,230);
            ActualW=FMath::Max(ActualW,float(M.X));ActualH=FMath::Max(ActualH,float(I*Em*1.1f+M.Y));}
        if(FrontlineUIAssetsV8213::BaselineMode()||V36TypographyAuditActive)RecordTypographyV36(V,X,Y,W,H,ActualW,ActualH,Em,false,Align,X+(Align==1?(W-ActualW)*.5f:Align==2?W-ActualW:0.f),CY,RoleInfoV8210(R,V,Em,230).FontObject->GetPathName(),&Rows);
    }
