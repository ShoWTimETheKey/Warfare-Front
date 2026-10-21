// Tactical-camera lighting, not an astronomical or historical weather claim.
// Existing native rainfall, wetness, mist and gameplay visibility remain intact.
#pragma once
namespace WarfareFRRULightingV87
{
struct FKey {const char* Mission;float Sun,Sky,Pitch,Yaw,Angle,Cloud,Backdrop;FString StableName=UTF8_TO_TCHAR(Mission);};
inline const FKey Keys[]={
 {"fr.mort_homme",1.17f,.89f,-23,-18,2.7f,.28f,.98f},
 {"fr.vaux",1.10f,.92f,-28,-23,3.2f,.20f,.98f},
 {"fr.douaumont",1.21f,.88f,-24,-17,2.5f,.38f,1.f},
 {"fr.louvemont",.84f,.87f,-16,-22,3.5f,0.f,.92f},
 {"fr.chemin_dames",1.08f,.93f,-25,-20,3.6f,0.f,.98f},
 {"fr.malmaison",1.22f,.88f,-20,-16,2.6f,.32f,.97f},
 {"fr.matz",1.23f,.90f,-29,-23,2.4f,.38f,1.f},
 {"fr.champagne_defence",1.10f,.94f,-25,-19,3.7f,0.f,.98f},
 {"fr.soissons",1.28f,.88f,-28,-24,2.2f,.42f,1.f},
 {"fr.sommepy",1.19f,.89f,-23,-21,2.8f,.28f,.97f},
 {"ru.galicia",1.26f,.89f,-28,-22,2.3f,.36f,1.f},
 {"ru.przemysl",1.03f,.92f,-21,-18,3.4f,0.f,.96f},
 {"ru.gorlice",1.18f,.90f,-29,-25,2.8f,.28f,.98f},
 {"ru.warsaw_rearguard",1.12f,.93f,-26,-17,3.4f,.16f,.98f},
 {"ru.naroch",.87f,.88f,-16,-22,4.1f,0.f,.92f},
 {"ru.lutsk",1.20f,.90f,-28,-21,2.6f,.30f,1.f},
 {"ru.czernowitz",1.16f,.91f,-25,-19,3.1f,.25f,.99f},
 {"ru.kovel",1.02f,.95f,-27,-23,4.2f,0.f,.96f},
 {"ru.kerensky",1.22f,.89f,-25,-24,2.5f,.34f,1.f},
 {"ru.riga",1.07f,.93f,-22,-18,3.7f,0.f,.96f}
};
inline bool Apply(const wfexp::Mission& Mission,FFrontlineWeatherLookV50& L)
{
 for(const auto& K:Keys)if(std::string_view(Mission.id)==K.Mission)
 {
  L.bAuthoredLightingV84=true;L.LightProfileKeyV842=*K.StableName;
  L.LightSeasonMonthV842=(Mission.date/100)%100;L.LightPhaseV842=TEXT("Day");
  L.SunScale=K.Sun;L.SkyScale=K.Sky;L.Pitch=K.Pitch;L.Yaw=K.Yaw;L.SourceAngle=K.Angle;
  L.bCloudShadowV84=K.Cloud>0.f;L.CloudShadowScaleV84=K.Cloud;L.BackdropScale*=K.Backdrop;
  return true;
 }
 return false;
}
}
