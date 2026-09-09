"""Human-maintained reference inventory, independent from the Blender generator.

Dimensions are design envelopes (width, depth, height), in metres. Uncaptioned
dimensions are inferred design targets, not claims of measurements from a drawing.
Multiple reference addresses mean one reusable asset, never a second camera view.
"""
import json
from pathlib import Path

PILOTS = {"canal_wall", "bridge_arch", "stairs_medium", "railing_iron", "lantern_amber", "door_standard"}

# ID | English label | production stage | W,D,H | sheet:section:variant references
ROWS = """
cathedral_wall_plain|Cathedral wall, base|3|2,0.75,6|1:1:base
cathedral_wall_window|Cathedral wall with window|3|2,0.75,6|1:1:window
cathedral_wall_banner|Cathedral wall with banner|3|2,0.75,6|1:1:banner
cathedral_corner|Cathedral corner|3|0.75,0.75,6|1:1:corner
cathedral_buttress|Cathedral buttress|3|0.75,1,6|1:1:buttress
tower_high|High cathedral tower|3|2.5,2.5,10|1:2:tower/front/back/plan
bridge_arch|Arched bridge|1|6,2.5,4|1:bridge:main/front
bridge_side|Bridge side segment|2|1,0.5,3.75|1:bridge:side
canal_wall|Standard canal wall|1|3,0.75,2.75|1:4:main;2:5:standard_edge
canal_corner|Canal corner|2|1,1,2.75|1:4:corner
canal_end|Canal end joint|2|0.75,0.75,2.75|1:4:end
mooring_pillar|Mooring pillar|2|0.5,0.5,2.75|1:4:pillar;2:5:mooring_post
stairs_grand|Grand two-flight staircase|2|4,6,4.2|1:stairs:main/front/side/detail
stairs_small|Main staircase, 6 steps|2|3,2.25,1.5|2:1:small
stairs_medium|Main staircase, 9 steps|1|3,3.375,2.25|2:1:medium
stairs_large|Main staircase, 12 steps|2|3.5,4.5,4.2|2:1:large;2:7:orthographic_example;2:8:large_stair
house_small_civic|Small civic house|3|3,3,3.5|1:6:small
house_medium_civic|Medium civic house|3|3.5,3.5,6|1:6:medium
house_high_civic|High civic house with crane|3|3.5,3.5,9|1:6:high
door_low|Low residential door|3|1.25,0.4,2|1:7:low
door_standard|Standard gothic door|1|1.75,0.45,3|1:7:standard
door_cathedral|High cathedral door|3|3,0.5,5.5|1:7:high
window_narrow|Narrow lancet stained glass|3|1,0.2,3|1:8:narrow;3:4:narrow
window_rose_tall|Tall rose stained glass|3|3,0.2,5|1:8:rose
window_medium|Medium lancet stained glass|3|1.5,0.2,3|1:8:medium
balcony_simple|Simple balcony|2|2,1.5,1.25|1:9:simple
balcony_extended|Extended balcony|2|3,1.5,1.25|1:9:extended
gallery|Elevated gallery|2|3,1.25,1.5|1:9:gallery
lift_platform|Hoisting platform|4|1.5,1.5,2|1:9:platform
awning_shop|Shop awning with counter|4|2.5,1.75,2.5|1:9:shop;3:3:market_stall
canal_arch|Plain canal arch|2|4,0.75,3|1:10:plain;2:5:arched_drain
canal_grated_arch|Grated canal arch|2|4,0.75,3|1:10:grated;2:5:drain_grate
canal_sluice|Lifting sluice gate|2|4.5,1,3.5|1:10:sluice;2:5:sluice
canal_arch_pier|Canal arch side pier|2|0.75,0.75,3|1:10:side
scale_rat_small|Static small rat scale figure|5|0.4,0.6,0.9|1:11:small;2:8:small;3:8:small
scale_rat_medium|Static medium rat scale figure|5|0.65,0.8,1.7|1:11:medium;2:8:medium;3:8:medium
scale_rat_high|Static tall rat scale figure|5|0.85,1,2.35|1:11:high;2:8:high;3:8:high
ladder_wood|Overlay wooden ladder|2|0.75,0.18,2.5|2:2:wood/placed/orthographic
ladder_metal|Overlay metal ladder|2|0.75,0.15,2.5|2:2:metal/placed/orthographic
platform_side|Side landing|2|2,1.5,1|2:3:side
passage_service|Narrow service passage|2|4,1,1.25|2:3:service
platform_corner|Corner landing|2|2,2,1|2:3:corner
platform_cantilever|Cantilever landing|2|2,1.5,1.5|2:3:cantilever
passage_stairs|Passage between stairs|2|3,1.5,1.5|2:3:stairs
passage_suspended|Suspended passage|2|3,1.5,2.5|2:3:suspended
parapet_short|Short masonry parapet|2|1.5,0.5,1.25|2:4:short
parapet_long|Long masonry parapet|2|3,0.5,1.25|2:4:long
railing_iron|Wrought iron railing|1|3,0.25,1|2:4:iron
post_lantern|Post with lantern|2|0.5,0.5,2|2:4:lantern_post
barrier_chain|Chain barrier|2|2,0.25,1|2:4:chain
mooring_ring|Mooring ring|2|0.3,0.15,0.4|2:4:ring
post_decorative|Decorative post|2|0.3,0.3,1.5|2:4:decorative
post_corner|Corner post|2|0.5,0.5,1.5|2:4:corner
post_end|End post|2|0.5,0.5,1.25|2:4:end
canal_wall_parapet|Canal edge with parapet|2|3,0.75,3.75|2:5:parapet_edge
outfall_low|Low outfall, dry architecture|2|1.5,0.75,2|2:5:low_outfall
outfall_high|High outfall, dry architecture|2|1.5,0.75,3|2:5:high_outfall
ladder_water|Water access ladder|2|0.75,0.25,3|2:5:water_ladder;1:4:main_ladder
pier_wood|Wooden pier|2|3,2,1|2:5:pier
waterlevel_wall|Retaining wall with level rings|2|2,0.75,4|2:waterlevel:section
waterlevel_marks|Water level marks|2|0.4,0.15,3|2:waterlevel:marks
crane_port|Large port crane|4|3,2,4|2:6:large
crane_wall|Small wall crane|4|2,0.75,2.5|2:6:small
cargo_cage|Suspended cargo cage|4|1.5,1.5,2|2:6:cage
cargo_crate|Large cargo crate|4|1.25,1.25,1.25|2:6:crate
barrel_pallet|Barrels on pallet|4|1.5,1.25,1.25|2:6:pallet
house_small_adapted|House for small inhabitants|3|3,3,4.5|3:1:small
house_mixed_adapted|House for mixed population|3|3.5,3.5,6.5|3:1:medium
house_high_adapted|House with tall-inhabitant addition|3|3.5,3.5,8.5|3:1:high/height_comparison
door_small_adapted|Small adapted door|3|0.8,0.35,1.2|3:2:small
door_medium_adapted|Medium adapted door|3|1.1,0.35,1.6|3:2:medium
door_high_adapted|Tall adapted door|3|1.5,0.4,2.4|3:2:high
bench|Bench|4|1.75,0.6,1|3:3:bench
table|Table|4|1.75,1,0.9|3:3:table;3:8:furniture_comparison
stool|Stool|4|0.4,0.4,0.5|3:3:stool
altar|Altar|4|1.5,0.75,1.5|3:3:altar
coffin|Wooden coffin|4|0.75,2,0.6|3:3:coffin
sarcophagus|Stone sarcophagus|4|1,2.25,1|3:3:sarcophagus
bookshelf|Bookcase|4|1.5,0.5,2.5|3:3:bookshelf
noticeboard|Notice board|4|1.75,0.35,2|3:3:noticeboard
shrine|Street shrine|4|1,0.6,2.5|3:3:shrine
undertaker_counter|Undertaker office counter|4|2.5,0.8,1.25|3:3:office
lantern_amber|Amber lantern|1|0.35,0.35,0.75|3:3:light/head;1:4:lamp;2:4:lamp
light_standard|Standard standing lantern|5|0.5,0.5,2|3:3:light/left
light_high|Tall standing lantern|5|0.5,0.5,2.5|3:3:light/middle
light_short|Short standing lantern|5|0.4,0.4,1.5|3:3:light/right
awning_fabric|Separate cloth awning|4|2.5,1.5,2.5|3:3:awning
clothesline|Clothes line|4|3,0.3,2|3:3:clothes
barrel|Barrel|4|0.65,0.65,1|3:3:barrels
crate_small|Small crate|4|0.65,0.65,0.65|3:3:crates/small
crate_medium|Medium crate|4|1,0.8,0.8|3:3:crates/medium
handcart|Hand cart|4|1.25,2.5,1|3:3:cart
boat|Canal rowboat|4|1.25,3.5,0.8|3:3:boat
water_pump|Water pump|4|0.75,0.75,2|3:3:pump
window_high|Tall lancet stained glass|3|1.5,0.2,4|3:4:high
window_rose_square|Square rose stained glass|3|3,0.2,3|3:4:rose
window_wide|Wide lancet stained glass|3|2,0.2,3|3:4:wide
gable|Gothic gable|3|1.5,0.5,2|3:4:gable
bracket_ornate|Ornate wall bracket|3|0.5,0.75,1.5|3:4:bracket
spire|Roof spire|3|0.4,0.4,2|3:4:spire
weathervane|Faith weathervane|3|0.6,0.15,1.5|3:4:weathervane
window_niche|Window niche|3|1,0.5,1.75|3:4:niche
cornice|Cornice|3|1.5,0.5,0.5|3:4:cornice
corbel|Corbel|3|0.5,0.5,1|3:4:corbel
pinnacle|Pinnacle|3|0.4,0.4,1.5|3:4:pinnacle
statue_repose|Rat statue, repose|5|1,1,3|3:5:repose
statue_vigil|Rat statue, vigil|5|1,1,3|3:5:vigil
statue_bowed|Rat statue, bowed|5|1,1,3|3:5:bowed
mausoleum_front|Mausoleum entrance module|3|4,1,4.5|3:6:front
mausoleum_corner|Mausoleum corner|3|1,1,4.5|3:6:corner
mausoleum_side|Mausoleum side segment|3|2,0.75,4.5|3:6:side
crypt_steps|Crypt entry steps|3|2.5,1.5,0.75|3:6:steps
grave_slab|Grave slab|3|1,2.25,0.3|3:6:slab
crypt_arch|Crypt arch|3|1.75,0.6,3|3:6:arch
crypt_dome|Crypt roof dome|3|2.5,2.5,2|3:6:dome
sign_tavern|Tavern sign|5|0.75,0.3,1|3:7:tavern
sign_shop|Shop sign|5|0.6,0.4,1|3:7:shop
sign_undertaker|Undertaker sign|5|0.6,0.3,1|3:7:undertaker
sign_temple|Temple symbol|5|0.75,0.2,1.25|3:7:temple
banner_narrow|Narrow banner|5|1,0.2,3|3:7:narrow
banner_medium|Medium banner|5|1.5,0.2,4|3:7:medium;1:1:banner/cloth
banner_large|Large banner|5|2,0.2,4|3:7:large
"""

CAPTIONED = {
    "house_small_civic", "house_medium_civic", "house_high_civic", "door_low", "door_standard",
    "door_cathedral", "window_narrow", "window_rose_tall", "window_medium", "canal_arch",
    "canal_grated_arch", "canal_sluice", "scale_rat_small", "scale_rat_medium", "scale_rat_high",
    "stairs_large", "door_small_adapted", "door_medium_adapted", "door_high_adapted",
    "window_high", "window_rose_square", "window_wide", "banner_narrow", "banner_medium", "banner_large",
}

# Latest dimensioned sheets supersede the earlier unmeasured art. Tuple order
# is width, depth, height. None is explicitly unreadable/unlabelled, not zero.
MEASURED = {
    "cathedral_wall_plain":(4,1,9),"cathedral_wall_window":(4,1,9),"cathedral_wall_banner":(4,1,9),
    "cathedral_corner":(2,2,9),"cathedral_buttress":(2,2,9),"tower_high":(6,6,15),
    "bridge_arch":(8,None,5),"canal_wall":(8,4,4.5),"canal_corner":(2,4,4.5),"canal_end":(2,4,4.5),"mooring_pillar":(1.5,1.5,4.5),
    "stairs_grand":(8,6,4.5),"stairs_small":(2.4,2,1.2),"stairs_medium":(2.4,2,1.8),"stairs_large":(2.4,2,2.4),
    "house_small_civic":(5,5,6),"house_medium_civic":(6,6,8),"house_high_civic":(8,7,10),
    "door_low":(1.2,.3,2.5),"door_standard":(1.8,.4,3.5),"door_cathedral":(2.4,.5,4),
    "window_narrow":(1,.3,3),"window_rose_tall":(2.5,.3,3.5),"window_medium":(1.2,.3,3),
    "balcony_simple":(3,1.5,3),"balcony_extended":(6,1.5,3),"gallery":(6,2,3.5),"lift_platform":(2,2,4),
    "awning_shop":(3,2,2.5),"canal_arch":(4,4,4),"canal_grated_arch":(4,4,4),"canal_sluice":(5,4,4.5),"canal_arch_pier":(2,4,4.5),
    "ladder_wood":(.8,None,2.4),"ladder_metal":(.8,None,2.4),
    "platform_side":(2,2,1),"passage_service":(3,1.5,1),"platform_corner":(2,2,1),"platform_cantilever":(2,1.5,1),"passage_stairs":(3,1,1),"passage_suspended":(3,1.5,1),
    "parapet_short":(2,None,1),"parapet_long":(3,None,1),"railing_iron":(2,None,1),"post_lantern":(.6,.6,None),
    "barrier_chain":(2,None,.8),"mooring_ring":(.4,None,.6),"post_decorative":(.4,None,1),"post_corner":(.5,None,1),"post_end":(.5,None,1),
    "canal_wall_parapet":(2,2,2.5),"outfall_low":(2,1,3),"outfall_high":(2,1,4),
    "crane_port":(5,1.5,4),"crane_wall":(3,1,3),"cargo_cage":(1.5,None,3),"cargo_crate":(1,1.2,1),"barrel_pallet":(.6,None,.8),
    "house_small_adapted":(4,3.5,5.5),"house_mixed_adapted":(5,4.5,7.3),"house_high_adapted":(6,5.5,9.8),
    "door_small_adapted":(.8,.6,2),"door_medium_adapted":(1.2,.6,2.8),"door_high_adapted":(1.6,.6,3.6),
    "bench":(1.8,.5,.9),"table":(1.2,.7,.8),"stool":(.35,.35,None),"altar":(1.2,1,1.8),"coffin":(2.2,.8,.6),"sarcophagus":(2.4,1.4,1),
    "bookshelf":(1.2,.4,2),"noticeboard":(1.4,.3,1.6),"shrine":(1,.8,1.8),"undertaker_counter":(2,.8,1.1),
    "light_standard":(.6,.6,2.5),"light_high":(.6,.6,2.5),"light_short":(.6,.6,2.5),
    "awning_fabric":(None,1,1.8),"clothesline":(3,None,1.6),"handcart":(None,3,.6),"water_pump":(.6,.8,1.6),
    "window_high":(1.2,None,3),"window_rose_square":(3,None,3),"window_wide":(2,None,2.5),
    "bracket_ornate":(None,None,1.8),"spire":(None,None,3.4),"window_niche":(1,None,1.8),"cornice":(None,None,.8),"corbel":(1.2,.6,1.2),"pinnacle":(.5,.5,2),
    "statue_repose":(1.2,1.2,4),"statue_vigil":(1.2,1.2,4),"statue_bowed":(1.2,1.2,4),
    "mausoleum_front":(6,4,4.5),"mausoleum_corner":(2,2,2.8),"mausoleum_side":(2,2,2.8),"crypt_steps":(3,2,.4),"grave_slab":(2,1.2,.4),"crypt_arch":(2,1,2.4),"crypt_dome":(2,2,3),
    "sign_tavern":(.8,None,1.2),"sign_shop":(.6,None,1),"sign_undertaker":(.7,None,1.5),"sign_temple":(.8,None,1.6),
    "banner_narrow":(1,None,3),"banner_medium":(1.5,None,4),"banner_large":(2,None,4),
}

CONFLICTS = {
    "tower_high":"Main/plan: 6x6x15; side view: 4x4x12. Main envelope wins; one asset.",
    "bridge_arch":"Main overall height 5; front 3 interpreted as walking/arch structure datum. Deck top 3.1 including paving; depth 3 inferred.",
    "stairs_grand":"Main 8x6x4.5; inset 4x3x2, step .18/.30. Overall envelope wins; pending detailed layout.",
    "stairs_small":"6 risers and H1.2, D2 win over .40 tread; derived tread 2/6.",
    "stairs_medium":"9 risers and H1.8, D2 win over .40 tread; derived tread 2/9.",
    "stairs_large":"12 risers and H2.4, D2 win over .40 tread; derived tread 2/12. Main W2.4 wins over orthographic W2.",
    "window_high":"Sheet3 arrow H3 wins over caption H3.5; W1.2.",
    "outfall_low":"Outer W2 and opening W2 are inconsistent; outer envelope wins, opening must allow jambs.",
    "outfall_high":"Outer W2 and opening W2 are inconsistent; outer envelope wins, opening must allow jambs.",
    "scale_rat_small":"Different sheets illustrate .8/1.0; approved canonical figure remains .9 (range midpoint).",
    "scale_rat_medium":"Different sheets illustrate 1.6/1.8; approved canonical figure remains 1.7 (range midpoint).",
    "scale_rat_high":"Different sheets illustrate 2.2/2.4/2.5; approved canonical figure remains 2.35 (range midpoint).",
}


def inventory():
    assets = []
    for line in ROWS.strip().splitlines():
        asset_id, label, stage, dimensions, refs = line.split("|")
        clips = ["open", "close"] if asset_id.startswith("door_") or asset_id == "canal_sluice" else []
        if asset_id.startswith("crane_"):
            clips = ["rotate", "lift", "lower"]
        if asset_id == "lift_platform":
            clips = ["lift", "lower"]
        if asset_id in {"cargo_cage", "clothesline", "awning_fabric", "awning_shop"} or asset_id.startswith("banner_"):
            clips = ["sway"]
        assets.append({
            "id": asset_id, "label": label, "stage": int(stage), "references": refs.split(";"),
            "design_dimensions_m": list(map(float, dimensions.split(","))),
            "dimension_basis": "captioned height/width; remaining axes inferred" if asset_id in CAPTIONED else "inferred from adjacent reference objects",
            "source_status": "pending", "native_status": "pending_A1_qualification",
            "source": None, "export": None, "materials": [], "animations": [], "planned_clips": clips,
        })
    assert len({a["id"] for a in assets}) == len(assets)
    # Revised sheets add genuinely different small modules; retain omitted old
    # assets and do not collapse newly captioned silhouettes into large modules.
    extras=[
        ("canal_wall_compact","Compact canal edge",2,(2,2,2),["2:5:standard_edge"]),
        ("sluice_compact","Compact lifting sluice",2,(4,1,3),["2:5:sluice"]),
        ("ladder_wood_short","Short wooden overlay ladder",2,(.8,.18,1.2),["2:2:wood/short"]),
        ("ladder_metal_short","Short metal overlay ladder",2,(.8,.15,1.2),["2:2:metal/short"]),
        ("window_narrow_tall","Narrow stained glass, tall adaptation",3,(1,.3,3.5),["3:4:narrow"]),
        ("market_stall_compact","Compact market stall",4,(2,1.2,2.2),["3:3:market_stall"]),
    ]
    for key,label,stage,dims,refs in extras:
        assets.append({"id":key,"label":label,"stage":stage,"references":refs,"design_dimensions_m":list(dims),"dimension_basis":"latest dimensioned sheet; unknown small depths inferred","source_status":"pending","native_status":"pending_A1_qualification","source":None,"export":None,"materials":[],"animations":[],"planned_clips":["open","close"] if key=="sluice_compact" else []})
    removed_refs={"canal_wall":"2:5:standard_edge","canal_sluice":"2:5:sluice","window_narrow":"3:4:narrow","awning_shop":"3:3:market_stall"}
    for asset in assets:
        key=asset["id"]
        if key in removed_refs:
            asset["references"].remove(removed_refs[key])
        if key in MEASURED:
            asset["reference_dimensions_m"]=list(MEASURED[key])
            asset["design_dimensions_m"]=[ref if ref is not None else prior for ref,prior in zip(MEASURED[key],asset["design_dimensions_m"])]
            asset["dimension_basis"]="latest dimensioned sheet; null reference axes retain explicitly inferred design targets"
        if key in CONFLICTS:
            asset["dimension_conflict"]=CONFLICTS[key]
        if key.startswith("door_"):
            asset["opening_dimensions_m"]=asset["design_dimensions_m"].copy()
            asset["dimension_extent"]="clear opening; frame increases decorated envelope"
        if key.startswith("stairs_"):
            asset["dimension_extent"]="walking structure; railings add height"
    next(a for a in assets if a["id"]=="sluice_compact")["dimension_conflict"]="Outer W4/H3 also labelled clear opening with 1m pylons. Outer envelope wins; clear aperture must be smaller."
    next(a for a in assets if a["id"]=="window_narrow_tall")["dimension_conflict"]="Arrow H3.5 wins over caption H2.5."
    return {"schema_version": 1, "units": "metres", "grid_m": 1, "detail_grid_m":0.25,
            "dimension_policy":"Latest main-view envelope and step count take precedence over contradictory inset details; all conflicts remain recorded. Door dimensions are openings. Unlabelled axes are explicit inferred targets.",
            "reference_source": "Three user-supplied Canal City of Ratfolk concept sheets plus latest dimensioned revisions, conversation 2026-09-09; original bitmap files not available in checkout",
            "deduplication": "References use sheet:section:variant. Repeated views share a record; substantially different silhouettes/dimensions remain separate variants. Composite scenes use existing parts.",
            "exclusions": ["Waterfall/flame effects: architectural outfalls and lamp meshes only", "Scale scenes reuse figures and furniture", "Water levels are illustrative placement guides, not separate fluids"],
            "assets": assets}


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists():
        parser.error("Output already exists; choose a new path to protect edited manifests")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(inventory(), indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
