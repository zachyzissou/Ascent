# ClimbingGame - Core Gameplay Mechanics Specification

## Overview

This document provides detailed specifications for ClimbingGame's core gameplay mechanics, building upon the foundation established in the Game Design Document, Implementation Roadmap, and Technical Architecture. It focuses on player interactions, system interconnections, and concrete gameplay scenarios that demonstrate how systems work together to create engaging climbing experiences.

---

## 1. Tool-Based Climbing System

### 1.1 Tool Categories and Interactions

#### Primary Protection Tools

**Cams (Spring-Loaded Camming Devices)**
- **Placement Mechanics**:
  - Must be placed in parallel-sided cracks (crack width: 0.5" to 6")
  - Placement quality affected by rock type, crack condition, and player skill
  - Visual feedback system shows placement strength (Green/Yellow/Red indicators)
  - Requires 2-second placement time with stamina cost (-5 points)
  - Can be over-cammed (too tight) or under-cammed (too loose), affecting holding power

- **Durability System**:
  - 50 placements before significant wear
  - Each fall reduces durability by 2-5 points based on force
  - Visual wear indicators: new → scratched → worn → damaged
  - Damaged cams have 25% failure chance under heavy loads

- **Gameplay Impact**:
  - Perfect placements hold 2000kg forces
  - Poor placements fail at 800kg forces
  - Walking on cams (gentle movement) preserves gear
  - Dynamic falls generate 600-1200kg forces based on fall factor

**Nuts/Stoppers**
- **Placement Mechanics**:
  - Must be placed in constricting cracks or behind constrictions
  - Requires precise sizing - too small falls out, too large won't set properly
  - 1-second placement time, minimal stamina cost (-2 points)
  - Can be improved with gentle testing tugs

- **Advanced Techniques**:
  - Micro-nuts for thin cracks (4-8mm) - high skill requirement
  - Opposition placements using two nuts create bomber anchors
  - Can be enhanced with slings to reduce rope drag

**Pitons**
- **Placement Requirements**:
  - Requires hammer tool (adds 500g to pack weight)
  - 8-15 second placement time with high stamina cost (-20 points)
  - Creates permanent damage to rock (environmental consequence)
  - Ring when properly set (audio feedback system)

- **Strategic Use**:
  - Essential for aid climbing routes
  - Emergency placements when other gear fails
  - Historical routes may require pitons for authentic ascents

#### Rope Systems and Applications

**Dynamic Ropes (Sport/Trad Climbing)**
- **Physics Properties**:
  - 30-35% stretch under load reduces impact forces
  - Elongation creates safer catches but more rope stretch
  - Better for lead climbing and long falls

**Static Ropes (Aid/Rescue)**
- **Applications**:
  - Hauling heavy loads (efficiency: 95% vs 70% for dynamic)
  - Fixed lines for multiple ascents
  - Rescue operations requiring precise positioning

**Advanced Rope Techniques**:

*Lead Climbing Sequence*:
1. Climber places protection every 3-8 feet
2. Clips rope through quickdraws or slings
3. Fall protection calculated by distance above last piece
4. Fall factor = fall distance ÷ rope length in system
5. Higher fall factors create more dangerous forces

*Multi-Pitch Management*:
1. Leader climbs full rope length (60m)
2. Builds belay anchor using 3+ pieces in SERENE system:
   - **S**olid placements
   - **E**qualized load distribution  
   - **R**edundant backup pieces
   - **E**fficient rope management
   - **N**o extension if piece fails
3. Belays second climber up
4. Repeats for multiple pitches

#### Specialized Tools

**Grappling Hook System**
- **Mechanics**:
  - 15-meter throwing range with accuracy decreasing by distance
  - Success rate: 90% at 5m, 60% at 10m, 30% at 15m
  - Skill progression improves accuracy and reduces stamina cost
  - Can only hook positive features (ledges, blocks, trees)
  - Not suitable for thin cracks or smooth walls

- **Tactical Applications**:
  - Accessing starts of difficult routes
  - Emergency retreat from dangerous positions
  - Creative problem-solving on overhanging terrain

**Pulley Systems**
- **Mechanical Advantage Types**:
  - Simple 2:1 system: halves force required but doubles distance
  - Complex 3:1 system: one-third force but triple distance
  - Compound systems possible for extreme loads

- **Rescue Applications**:
  - Hauling injured climbers
  - Lifting heavy haul bags on big walls
  - Creating tension for Tyrolean traverses

### 1.2 Tool Combination Strategies

#### Equalized Anchors
**The SERENE System Implementation**:
- Minimum 3 pieces of protection
- Load distribution using cordelette or slings
- Master point created for single attachment
- If one piece fails, load redistributes to remaining pieces
- Visual indicator shows load distribution across pieces

#### Tool Synergies
- **Cam + Nut Combination**: Cam in back of crack, nut in constriction
- **Extended Placements**: Use slings to reduce rope drag and prevent walking
- **Opposition Systems**: Two pieces pulling against each other create bomber placements

---

## 2. Stamina and Injury Management System

### 2.1 Multi-Layer Stamina System

#### Base Stamina (100 points)
**Consumption Rates**:
- **Free Climbing**: -2/second on moderate holds, -4/second on bad holds
- **Aided Climbing**: -1/second with good gear, -2/second on marginal gear
- **Dynamic Moves**: -20 points per dyno or jump move
- **Hanging**: -3/second on good jugs, -6/second on crimps
- **Tool Placement**: -5 cams, -2 nuts, -20 pitons, -3 clipping quickdraws

**Recovery Rates**:
- **Good Holds/Ledges**: +5/second recovery
- **Shake-Outs**: +3/second while hanging and shaking limbs
- **Resting**: +8/second when sitting/lying on ledge
- **Belaying**: +2/second while belaying partner (partial recovery)

#### Pump System (Secondary Stamina Layer)
**Mechanics**:
- Develops when stamina drops below 60 points
- Reduces grip strength: 100% at 0 pump → 50% at maximum pump
- Recovery requires complete rest (no climbing movement)
- Takes 30-60 seconds to fully recover from severe pump

**Gameplay Impact**:
- Pump forces strategic rest planning
- Creates tension as climber searches for rest positions
- Partners can provide encouragement (minor pump reduction bonus)

#### Environmental Modifiers
- **Temperature Effects**:
  - Cold (-5°C to 5°C): 15% stamina penalty, improved grip on holds
  - Hot (25°C to 35°C): 25% stamina penalty, worse grip from sweaty hands
  - Extreme Cold (<-5°C): 30% penalty, frostbite risk after 10 minutes
  - Extreme Heat (>35°C): 35% penalty, heat exhaustion possible

- **Altitude Effects**:
  - Sea level to 1000m: No effect
  - 1000m to 2500m: 10% stamina penalty
  - 2500m to 4000m: 25% penalty, slower recovery
  - Above 4000m: 40% penalty, altitude sickness risk

### 2.2 Injury System with Gameplay Consequences

#### Fall Damage Calculation
**Factors**:
- Fall distance (linear damage increase)
- Landing surface (rock = full damage, snow = 50% damage, water = 25% damage)
- Protection effectiveness (good pro = 25% damage, no pro = full damage)
- Fall factor (distance fallen ÷ rope length in system)

#### Injury Types and Effects

**Minor Injuries (Heal in 5-15 minutes)**:
- **Scrapes**: 5% movement speed penalty
- **Bruises**: 10% grip strength reduction
- **Minor Rope Burn**: 15% rope handling speed penalty

**Moderate Injuries (Require first aid, heal in 30-60 minutes)**:
- **Sprained Ankle**: 25% movement speed, cannot perform dynamic moves
- **Muscle Strain**: 20% stamina capacity reduction
- **Deep Cuts**: Slow bleeding (-1 health/minute until treated)

**Severe Injuries (Require evacuation)**:
- **Fractured Bone**: Cannot climb, requires rescue
- **Head Injury**: Vision effects, disorientation
- **Severe Blood Loss**: Unconsciousness risk, emergency evacuation needed

#### Treatment and Recovery
**First Aid Items**:
- **Bandages**: Stop bleeding, treat cuts (-100g weight)
- **Pain Medication**: Reduce injury penalties by 50% for 20 minutes
- **Elastic Bandage**: Support sprained joints, reduce movement penalties
- **Emergency Bivy**: Prevent hypothermia during rescue waits

---

## 3. Cooperative Multiplayer Mechanics

### 3.1 Belaying System

#### Dynamic Belaying Mechanics
**Belay Devices**:
- **Tube Device**: Manual control, allows dynamic catches, requires skill
- **Assisted Braking**: Automatic locking, safer but more jarring catches
- **GriGri Style**: Semi-automatic, good for beginners

**Belaying Actions**:
- **Taking Up Slack**: Remove excess rope as climber ascends
- **Giving Slack**: Allow rope out for clips or movement
- **Dynamic Catch**: Soft catch that reduces impact force on climber
- **Hard Catch**: Static belay that stops fall immediately (higher forces)

**Communication System**:
- **Mandatory Calls**: 
  - "On belay?" / "Belay on!" (establish belay)
  - "Climbing!" / "Climb on!" (begin ascent)
  - "Slack!" (need more rope)
  - "Take!" (hold my weight on rope)
  - "Falling!" (emergency call)

#### Partner Positioning
**Belay Stance Quality**:
- **Bomber Stance**: Anchored to wall, optimal force direction
- **Good Stance**: Solid footing, good anchor
- **Marginal Stance**: Unstable position, risk of being pulled off

**Consequences of Poor Belaying**:
- Ground falls if too much slack given
- Hard catches causing injuries if belay too static
- Belay failure if stance compromised during hard catch

### 3.2 Assistance Mechanics

#### Spotting System
**Effectiveness Factors**:
- Spotter positioning (directly below = best protection)
- Multiple spotters increase effectiveness
- Spotter skill affects reaction time and technique
- Clear landing zone required for optimal spotting

**Spotting Results**:
- **Perfect Spot**: 75% damage reduction, controlled landing
- **Good Spot**: 50% damage reduction, stable landing  
- **Marginal Spot**: 25% damage reduction, some impact absorbed
- **Failed Spot**: Spotter and climber both take damage

#### Cooperative Problem Solving

**Human Anchor Techniques**:
- One climber serves as anchor point for partner
- Requires good stance and solid handholds
- Limited to body weight loads (80kg maximum)
- Allows access to otherwise unreachable holds

**Assisted Moves**:
- **Shoulder Stand**: Lower climber provides platform (+2m reach)
- **Tension Traverse**: Using rope tension to reach distant holds
- **Hand/Foot Boost**: Direct assistance for individual moves
- **Counterbalance**: Two climbers balance each other's weight

**Team Gear Management**:
- **Gear Passing**: Share tools mid-route
- **Rack Management**: Optimize gear distribution between partners
- **Resupply**: Pass gear from belayer to leader during climb

### 3.3 Rescue Operations

#### Self-Rescue Techniques
**Scenario**: Leader falls and cannot continue
1. **Assessment**: Determine injury severity and options
2. **Lower-Off**: Belay partner lowers injured climber to safety
3. **Rescue Rappel**: Set up rappel system to descend
4. **Haul System**: Use pulleys to raise/lower injured climber

#### Team Rescue Scenarios
**Multi-Pitch Rescue**:
1. Secure injured climber to anchor
2. Establish communication with rescue services
3. Set up complex pulley system for evacuation
4. Coordinate with helicopter or ground rescue team

**Emergency Bivouac**:
- Emergency shelter setup for overnight wait
- Hypothermia prevention
- Rationing water and food supplies
- Signaling for rescue (mirror, whistle, headlamp)

---

## 4. Player Progression and Skill Development

### 4.1 Skill Categories

#### Technical Skills
**Climbing Technique Progression**:
1. **Beginner (Levels 1-10)**:
   - Basic movement on easy terrain (5.0-5.6 grades)
   - Simple tool placements with visual guides
   - Standard belaying with assisted devices
   - Recovery from minor mistakes

2. **Intermediate (Levels 11-25)**:
   - Complex sequences on moderate terrain (5.7-5.10)
   - Efficient gear placement without guides
   - Advanced belaying techniques
   - Route reading and sequence planning

3. **Advanced (Levels 26-40)**:
   - Technical terrain mastery (5.11-5.12)
   - Innovative gear solutions
   - Rescue technique proficiency
   - Multi-pitch route management

4. **Expert (Levels 41-50)**:
   - Elite-level climbing (5.13+)
   - Tool modification and customization
   - Expedition planning and leadership
   - First ascent exploration

#### Physical Attributes
**Strength Development**:
- **Finger Strength**: Improves grip duration on small holds
- **Core Strength**: Better body positioning and control
- **Endurance**: Increased base stamina and recovery rate
- **Flexibility**: Access to wider range of body positions

**Training Methods**:
- **Hangboard Training**: Targeted finger strength development
- **Campus Board**: Power and dynamic movement improvement
- **Cardio Training**: Base fitness and recovery enhancement
- **Flexibility Work**: Range of motion improvement

#### Mental Skills
**Risk Assessment**:
- Improved ability to evaluate gear placements
- Better weather and condition assessment
- Route difficulty evaluation accuracy
- Emergency situation management

**Focus and Concentration**:
- Reduced stamina penalties under pressure
- Better performance in exposure situations
- Improved ability to work through pump and fatigue
- Enhanced route-reading abilities

### 4.2 Equipment Progression

#### Gear Unlocks
**Level-Based Progression**:
- **Levels 1-10**: Basic rack (cams, nuts, quickdraws, standard rope)
- **Levels 11-20**: Specialized tools (micro gear, long slings, static ropes)
- **Levels 21-30**: Advanced equipment (ice tools, aid gear, haul bags)
- **Levels 31-40**: Expert tools (custom gear, lightweight options)
- **Levels 41-50**: Prototype equipment (experimental high-performance gear)

#### Gear Customization
**Modification Options**:
- **Weight Reduction**: Titanium components, hollow construction
- **Durability Enhancement**: Protective coatings, reinforced stress points
- **Performance Optimization**: Improved gate action, better grip surfaces
- **Aesthetic Customization**: Color schemes, personal engravings

### 4.3 Route-Specific Progression

#### Difficulty Grading System
**Yosemite Decimal System Integration**:
- **Class 1-2**: Hiking and scrambling (tutorial areas)
- **Class 3**: Easy climbing with exposure (beginner routes)
- **Class 4**: Roped climbing on moderate terrain
- **Class 5.0-5.15**: Technical rock climbing with sub-grades

#### Style and Ethics Progression
**Climbing Styles Unlocked**:
- **Top-Rope**: Safest style for learning (Levels 1-5)
- **Sport Climbing**: Pre-placed bolts for protection (Levels 6-15)
- **Traditional**: Self-placed protection (Levels 16-30)
- **Aid Climbing**: Direct support from gear (Levels 25-40)
- **Free Solo**: No protection whatsoever (Levels 45-50, high risk)

---

## 5. Risk/Reward Balance Framework

### 5.1 Climbing Style Comparison

#### Conservative Approach
**Characteristics**:
- Heavy gear rack (5-8kg total weight)
- Over-protection (gear every 2-3 feet)
- Slow progress (50% of normal speed)
- Maximum safety margins

**Rewards**:
- Minimal fall risk
- Reduced stamina penalties from fear
- Higher success rate on difficult moves
- Better recovery from mistakes

**Penalties**:
- Increased gear weight reduces stamina efficiency
- Slower progress uses more daylight hours
- May not complete longer routes in available time
- Reduced style points for heavy protection

#### Aggressive Approach
**Characteristics**:
- Light gear selection (2-3kg total weight)
- Run-out climbing (10-15 feet between pieces)
- Fast progress (150% of normal speed)
- Minimal safety margins

**Rewards**:
- Faster route completion
- Reduced weight burden improves efficiency
- Higher style points for bold climbing
- Access to longer, more committing routes

**Risks**:
- Serious injury potential from longer falls
- Higher stamina costs due to fear and commitment
- Less margin for error on difficult moves
- Potential for ground falls or ledge impacts

### 5.2 Environmental Risk Factors

#### Weather Windows
**Good Conditions**:
- Clear skies, mild temperatures
- Light winds (<15 mph)
- Dry rock surfaces
- Stable barometric pressure

**Marginal Conditions**:
- Partly cloudy, temperature extremes
- Moderate winds (15-25 mph)
- Recent precipitation (damp rock)
- Unstable weather patterns

**Dangerous Conditions**:
- Storm approach, lightning risk
- High winds (>25 mph)
- Wet or icy rock surfaces
- Extreme temperatures

#### Time Management
**Daylight Considerations**:
- Route length vs. available daylight
- Approach and descent time calculations
- Emergency time buffers
- Seasonal daylight variation effects

**Consequences of Poor Planning**:
- Forced bivouac without proper gear
- Dangerous descent in darkness
- Hypothermia risk in cold conditions
- Search and rescue activation

### 5.3 Route Selection Risk/Reward

#### Route Difficulty vs. Player Skill
**Appropriate Challenge (Skill ± 1 grade)**:
- Optimal learning experience
- Reasonable success chance (70-80%)
- Moderate risk level
- Good progression rewards

**Sandbag Routes (Skill + 2-3 grades)**:
- High failure rate (30-50%)
- Significant injury risk
- Exceptional rewards if successful
- Potential for skill breakthrough

**Warm-up Routes (Skill - 2-3 grades)**:
- Near-guaranteed success (95%+)
- Minimal risk
- Confidence building
- Good for practicing techniques

---

## 6. Concrete Gameplay Scenarios

### 6.1 Scenario 1: Multi-Pitch Traditional Route

**Setting**: 5-pitch, 5.8 trad route on granite wall

**Players**: Two experienced climbers (Alex - Leader, Sam - Follower)

**Sequence**:

**Pitch 1 - The Approach**:
1. Alex studies route from ground, identifies key features
2. Racks gear efficiently: cams on right side, nuts on left
3. Sam sets up belay with tube device, anchored to ground
4. Alex begins climbing, places first cam at 15 feet (good stance)
5. Continues with natural placements every 6-8 feet
6. Alex reaches 60m belay ledge, builds SERENE anchor with 3 cams
7. Calls "Safe!" and brings Sam up on belay

**Pitch 2 - The Crux**:
1. Sam reaches belay, reorganizes rack while Alex prepares
2. Alex identifies crux sequence - thin crack with poor feet
3. Places solid cam below crux, attempts move sequence
4. Fails on first attempt, hangs on gear (-20 stamina, pump develops)
5. Rests 60 seconds to recover from pump
6. Second attempt successful, continues to next belay

**Decision Point**: Alex realizes next pitch is harder than expected
- **Conservative Option**: Rappel from current position (safe but incomplete)
- **Aggressive Option**: Continue with marginal protection (high risk, full completion)
- **Alex chooses middle ground**: Places extra gear, proceeds carefully

**Pitch 3 - Weather Change**:
1. Clouds moving in, wind increasing
2. Alex places gear more frequently as conditions deteriorate
3. Light rain begins - rock becomes slippery
4. Alex and Sam must choose: retreat or push through to summit
5. They decide conditions are too dangerous, begin retreat

**Retreat Sequence**:
1. Set up rappel anchors using some placed gear
2. Double-rope rappels down each pitch
3. Carefully remove most gear (some left for safety)
4. Reach ground as storm intensifies

**Outcomes**:
- Route incomplete but team safe
- Experience gained in decision-making
- Gear preserved for future attempts
- Weather assessment skills improved

### 6.2 Scenario 2: Sport Climbing Fall Sequence

**Setting**: Single-pitch 5.11a sport route with 8 bolts

**Players**: Emma (climber), Jake (belayer)

**Sequence**:

**Setup Phase**:
1. Emma studies route moves from ground
2. Jake sets up belay with GriGri device
3. Emma ties in, performs safety check
4. Standard climbing calls exchanged

**Ascent Attempt**:
1. Emma climbs smoothly through first 4 bolts
2. Reaches rest position at 5th bolt, shakes out arms
3. Identifies crux sequence between bolts 6-7
4. Clips bolt 6, attempts difficult move sequence
5. Falls on move, catches on rope

**Fall Analysis**:
- Fall distance: 4 feet (2 feet above bolt + 2 feet rope stretch)
- Jake provides good dynamic catch (soft belay)
- Emma takes minimal damage due to good protection
- Stamina cost: -15 points for fall, -5 for adrenaline

**Recovery and Success**:
1. Emma rests on rope to recover composure
2. Analyzes failed sequence, identifies better foot position
3. Attempts sequence again with revised technique
4. Successfully completes crux move
5. Continues to anchor, lowers off safely

**Learning Outcomes**:
- Falling practice builds confidence
- Technical problem-solving under pressure
- Belayer-climber communication during falls
- Recovery techniques from failed attempts

### 6.3 Scenario 3: Rescue Operation

**Setting**: Multi-pitch route, partner injured on 3rd pitch ledge

**Players**: Sarah (uninjured), Mike (injured ankle), distant rescue team

**Emergency Setup**:
1. Mike falls on easy terrain, lands awkwardly on ledge
2. Ankle injury prevents further climbing
3. Sarah assesses situation: injury severity, available resources
4. Cell phone call to rescue services (intermittent signal)

**Self-Rescue Attempt**:
1. Sarah sets up anchor system for lowering
2. Creates pulley system using available gear
3. Attempts to lower Mike one pitch
4. System works but extremely slow progress
5. Daylight fading, weather deteriorating

**Decision Point**: Continue self-rescue vs. wait for professional help
- **Risk of continuation**: Dangerous night rescue, equipment limitations
- **Risk of waiting**: Hypothermia, weather exposure
- **Sarah chooses**: Secure bivouac, signal for helicopter

**Emergency Bivouac**:
1. Sarah creates shelter with space blanket and rope
2. Treats Mike's ankle with elastic bandage
3. Rations remaining water and food
4. Sets up signaling system with headlamp

**Professional Rescue**:
1. Helicopter arrives at first light
2. Technical rescue team rappels to position
3. Mike evacuated in rescue harness
4. Sarah climbs out with rescue team assistance

**Post-Incident Analysis**:
- Importance of emergency equipment
- Communication and signaling systems
- Decision-making under stress
- Self-rescue vs. professional rescue choices

### 6.4 Scenario 4: Competition Speed Climbing

**Setting**: Standardized 15-meter speed wall with pre-set route

**Players**: Two climbers racing head-to-head

**Race Format**:
- Simultaneous start on parallel walls
- Identical route layout and holds
- Top-rope protection (no gear placement)
- Time stops when touching finish hold

**Race Sequence**:
1. **Preparation Phase**:
   - Climbers study route for optimal sequence
   - Practice specific move combinations
   - Optimize clothing and shoes for speed
   - Mental preparation and visualization

2. **Start Phase**:
   - Electronic start signal
   - Explosive first moves critical for momentum
   - No time for precise foot placement
   - Commits to practiced sequence

3. **Mid-Route**:
   - Maintaining flow and rhythm
   - No rest positions available
   - Small mistakes compound quickly
   - Peripheral awareness of competitor

4. **Finish**:
   - Final explosive moves to top
   - Precise timing on finishing hold
   - Electronic timing system records result

**Skills Emphasized**:
- Route memorization and optimization
- Explosive power and coordination
- Mental focus under pressure
- Consistency in execution

### 6.5 Scenario 5: Alpine Mixed Climbing

**Setting**: High-altitude mixed route (rock and ice) in winter conditions

**Players**: Two alpinists on expedition-style climb

**Environmental Factors**:
- Altitude: 3800m (stamina penalties active)
- Temperature: -15°C (grip and dexterity penalties)
- Avalanche conditions present
- Limited daylight window (8 hours)

**Climbing Sequence**:

**Alpine Start**:
1. 3:00 AM departure to optimize daylight
2. Approach by headlamp over glaciated terrain
3. Route finding challenging in low visibility
4. Team must stay roped due to crevasse danger

**Technical Mixed Section**:
1. Transition from snow to rock requires gear change
2. Ice tools and crampons for ice sections
3. Cams and nuts for rock protection
4. Frequent gear transitions slow progress

**Weather Window Closure**:
1. Clouds building by noon (4 hours climbing time used)
2. Team reaches high point but summit still distant
3. Wind increasing, snow beginning
4. Critical decision: push for summit or retreat?

**Strategic Retreat**:
1. Team recognizes dangerous conditions developing
2. Begins descent while weather still marginal
3. Multiple rappels over mixed terrain
4. Reaches safety as storm intensity peaks

**Expedition Skills Demonstrated**:
- Weather pattern recognition
- Risk assessment in alpine environment
- Technical mixed climbing skills
- Strategic decision-making under pressure

---

## 7. System Interconnection Examples

### 7.1 Stamina-Tool-Risk Integration

**Scenario**: Climber low on stamina approaching difficult section

**System Interactions**:
1. **Stamina System**: Current stamina 30/100, pump developing
2. **Tool System**: Heavy rack (6kg) further reducing efficiency
3. **Risk Assessment**: Poor gear placement options ahead
4. **Environmental**: Afternoon heat building (+25% stamina drain)

**Player Choices**:
- **Drop Gear**: Reduce weight but compromise safety margins
- **Rest Position**: Find ledge to recover but use precious time
- **Push Through**: Accept high risk for route completion
- **Retreat**: Abandon attempt to preserve safety

**Consequences Cascade**:
- Choice affects immediate climbing ability
- Impacts partner's safety and options
- Influences route completion chances
- Determines skill progression rewards

### 7.2 Multiplayer-Communication-Safety Integration

**Scenario**: Communication failure during leader fall

**System Interactions**:
1. **Communication System**: Radio failure, wind noise masking voice
2. **Multiplayer System**: Belayer cannot hear fall warning
3. **Safety System**: Delayed belay reaction increases fall severity
4. **Injury System**: Harder catch results in minor injury

**Adaptation Strategies**:
- **Visual Signals**: Hand gestures and body language
- **Rope Communication**: Tugs and pulls on belay rope
- **Pre-arranged Protocols**: Standard procedures for communication loss
- **Environmental Awareness**: Positioning for better sight lines

### 7.3 Progression-Risk-Reward Integration

**Scenario**: Player attempting route at skill limit

**System Interactions**:
1. **Progression System**: Route difficulty matches player's maximum grade
2. **Risk System**: High failure chance (40%) with injury potential
3. **Reward System**: Exceptional XP and unlock potential if successful
4. **Equipment System**: Gear selection critical for success chances

**Dynamic Balancing**:
- **Success**: Major skill progression, equipment unlocks, confidence boost
- **Failure**: XP loss, potential injury, equipment wear
- **Partial Success**: Some progression, lesson learned, specific skill improvement
- **Strategic Retreat**: Minimal XP, preserved safety, route knowledge gained

---

## Conclusion

This comprehensive gameplay mechanics specification provides the detailed framework for implementing ClimbingGame's core systems. The interconnected nature of these mechanics creates emergent gameplay where player decisions cascade through multiple systems, producing varied and engaging climbing experiences. The concrete scenarios demonstrate how these systems work together to create realistic climbing challenges that reward both technical skill and strategic thinking.

The progression from basic tool placement to complex multi-pitch adventures provides a learning curve that mirrors real climbing development, while the cooperative elements ensure that multiplayer experiences are meaningfully different from solo climbing. The risk/reward framework gives players agency in choosing their preferred climbing style while maintaining realistic consequences for their decisions.