#pragma once

#include <cstdint>

namespace sanyam {

enum class Category : uint8_t {
  Yama = 0,
  Niyama = 1,
  Antaraya = 2,  // Obstacle
  Sahabhuva = 3  // Symptom
};

struct SanyamItem {
  const char* id;
  Category category;
  const char* categoryLabel;
  const char* name;
  const char* benefit;
  const char* questions;
  const char* howToFollow;
  const char* opt1;
  const char* opt2;
  const char* opt3;
};

// 4 Patanjali Yoga Sutra Quotes
static const char* const kPatanjaliQuotes[] = {
    "Yoga is the cessation of the movements of the mind. Then there is abiding in the Seer's own form. - Patanjali (1.2-3)",
    "Non-violence, truthfulness, non-stealing, continence, and non-greed constitute the Yamas. - Patanjali (2.30)",
    "Austerity, self-study, and dedicating all actions to the Supreme constitute Yoga in practice. - Patanjali (2.1)",
    "By cultivating friendliness toward the happy, compassion for the unhappy, and equanimity toward all, calmness is retained. - Patanjali (1.33)"
};

constexpr int kTotalQuotes = static_cast<int>(sizeof(kPatanjaliQuotes) / sizeof(kPatanjaliQuotes[0]));

// 24 Core Contemplation Items (5 Yamas, 5 Niyamas, 9 Antarayas, 5 Sahabhuvas)
static const SanyamItem kItems[] = {
    // --- 5 YAMAS ---
    {
        "yama-1",
        Category::Yama,
        "YAMA 1/5",
        "Ahimsa (Non-violence)",
        "Cultivates compassion, eliminates fear, and unites you with creation.",
        "Is there aggression inside or are you calm inside? Did you hurt anyone with thought, speech, or action?",
        "Yoga is realizing everything is part of you. When everything is you, how can you harm anything?",
        "Meticulous",
        "Moderate",
        "Not at all"
    },
    {
        "yama-2",
        Category::Yama,
        "YAMA 2/5",
        "Satya (Truthfulness)",
        "Karma Siddhi: All actions and words come to fruition.",
        "Do you remember the non-changing as the only truth? Are you doing tapas of speech (words that do not ruffle the calm mind)?",
        "Pay attention to what is. Truth is what is unaffected by time. You cannot lie to yourself.",
        "Meticulous",
        "Moderate",
        "Not at all"
    },
    {
        "yama-3",
        Category::Yama,
        "YAMA 3/5",
        "Asteya (Non-stealing / Abundance)",
        "All wealth and prosperity comes effortlessly.",
        "Do you feel 'I wish I had that' (subtle stealing/greed)? Is any desire or comparison making you feverish?",
        "Not missing what you don't have right now. Contentment in one's own existence.",
        "Meticulous",
        "Moderate",
        "Not at all"
    },
    {
        "yama-4",
        Category::Yama,
        "YAMA 4/5",
        "Brahmacharya (Moderation / Higher Self)",
        "Brings boundless physical strength and deep creativity.",
        "Do you see yourself as infinite consciousness rather than just the physical body? Are you conserving your vital energy or squandering it on sensory traps?",
        "Brahma means vast. Move in bigger things rather than petty bodily desires.",
        "Meticulous",
        "Moderate",
        "Not at all"
    },
    {
        "yama-5",
        Category::Yama,
        "YAMA 5/5",
        "Aparigraha (Non-possessiveness)",
        "Freedom, lightness, and deep inner security.",
        "Are you hoarding things, money, or thoughts? Did you hold onto insults or compliments from people?",
        "Don't take negativity or garbage thrown by others. Downsize, declutter, live light.",
        "Meticulous",
        "Moderate",
        "Not at all"
    },

    // --- 5 NIYAMAS ---
    {
        "niyama-1",
        Category::Niyama,
        "NIYAMA 1/5",
        "Shaucha (Purity / Cleanliness)",
        "Mental clarity, sensory mastery, and inner purity.",
        "Is your stomach clean? Is the mind empty and centered? When feeling miserable, realize ashaucha (going away from Self) occurred.",
        "Maintain cleanliness of surroundings, body, and return swiftly to stillness when disturbed.",
        "Meticulous",
        "Moderate",
        "Not at all"
    },
    {
        "niyama-2",
        Category::Niyama,
        "NIYAMA 2/5",
        "Santosha (Contentment)",
        "Supreme, permanent peace and joy are attained.",
        "Are you grateful and unconditionally happy right now? Do you accept the present moment, people, and situation as they are?",
        "Contentment is a conscious practice. If you don't take a step to be happy, nothing will make you happy.",
        "Meticulous",
        "Moderate",
        "Not at all"
    },
    {
        "niyama-3",
        Category::Niyama,
        "NIYAMA 3/5",
        "Tapas (Self-Discipline / Penance)",
        "Strengthens willpower, stabilizes mind and body.",
        "Are you enduring necessary discomfort willingly for a higher purpose? Do you have mastery over your sensory cravings?",
        "Change whatever you can; when you cannot change it, accept and endure it with equanimity.",
        "Meticulous",
        "Moderate",
        "Not at all"
    },
    {
        "niyama-4",
        Category::Niyama,
        "NIYAMA 4/5",
        "Swadhyaya (Self-Study)",
        "Wisdom blooms and the presence of the Divine is felt.",
        "Are you observing your mind, emotions, and hidden motives? Who am I? Are you reading inspiring scriptures?",
        "Observe what is running through your mind without judgment. Become hollow and empty.",
        "Meticulous",
        "Moderate",
        "Not at all"
    },
    {
        "niyama-5",
        Category::Niyama,
        "NIYAMA 5/5",
        "Ishwarapranidhana (Surrender)",
        "Frees from ego and anxiety; leads directly into Samadhi.",
        "Did you offer actions and drop anxiety over results? Before sleep, do you surrender everything to the Divine?",
        "When feeling helpless, surrender the burden. Do your sadhana and keep your smile.",
        "Meticulous",
        "Moderate",
        "Not at all"
    },

    // --- 9 ANTARAYAS (OBSTACLES) ---
    {
        "antaraya-1",
        Category::Antaraya,
        "OBSTACLE 1/9",
        "Vyadhi (Illness / Physical Sickness)",
        "Physical equilibrium, vitality, and somatic health.",
        "Did physical disorder, illness, or bodily fatigue impede your practice or clarity today?",
        "Nourish your body with proper rest, balanced diet, gentle movement, and medical care.",
        "None",
        "Moderate",
        "Severe"
    },
    {
        "antaraya-2",
        Category::Antaraya,
        "OBSTACLE 2/9",
        "Styana (Mental Lethargy / Dullness)",
        "Mental sharpness, clarity, and inner aliveness.",
        "Was your mind sluggish, heavy, or feeling uninspired and resistant to reflection?",
        "Engage in conscious pranayama, take a brisk walk, or chant to clear heavy mental fog.",
        "None",
        "Moderate",
        "Severe"
    },
    {
        "antaraya-3",
        Category::Antaraya,
        "OBSTACLE 3/9",
        "Samshaya (Doubt / Skepticism)",
        "Steadfast faith, conviction, and inner stillness.",
        "Did doubt, cynicism, or endless questioning paralyze your efforts and peace of mind?",
        "Rely on direct experience rather than abstract rumination. Trust the practice.",
        "None",
        "Moderate",
        "Severe"
    },
    {
        "antaraya-4",
        Category::Antaraya,
        "OBSTACLE 4/9",
        "Pramaada (Carelessness / Negligence)",
        "Mindful vigilance and unwavering attentiveness.",
        "Did you slip into heedlessness, procrastination, or negligence of your core responsibilities?",
        "Cultivate continuous mindfulness in small daily acts. Slow down and act deliberately.",
        "None",
        "Moderate",
        "Severe"
    },
    {
        "antaraya-5",
        Category::Antaraya,
        "OBSTACLE 5/9",
        "Alasya (Physical Laziness)",
        "Vigor, prompt initiative, and sustained momentum.",
        "Did physical heaviness or inertia stop you from taking necessary purposeful action?",
        "Do not negotiate with bodily laziness. Count to five, stand up, and begin.",
        "None",
        "Moderate",
        "Severe"
    },
    {
        "antaraya-6",
        Category::Antaraya,
        "OBSTACLE 6/9",
        "Avirati (Sense Craving / Lack of Restraint)",
        "Freedom from compulsive desires and sensory enslavement.",
        "Did sensory desires, digital distractions, or appetites pull your focus away from stillness?",
        "Practice conscious delay and restraint. Witness the craving rise, peak, and dissolve.",
        "None",
        "Moderate",
        "Severe"
    },
    {
        "antaraya-7",
        Category::Antaraya,
        "OBSTACLE 7/9",
        "Bhrantidarshan (Living in Illusion / Fantasy)",
        "Sober discernment, truth-seeing, and reality grounding.",
        "Were you trapped in unrealistic fantasies, false assumptions, or distorted self-images?",
        "Check your projections against reality. Seek objective feedback and stay humble.",
        "None",
        "Moderate",
        "Severe"
    },
    {
        "antaraya-8",
        Category::Antaraya,
        "OBSTACLE 8/9",
        "Alabdha Bhumikatva (No Progress / Plateau)",
        "Patience, perseverance, and quiet inner resilience.",
        "Did you feel frustrated or disheartened by an apparent lack of spiritual or personal progress?",
        "Spiritual roots grow deep in the dark. Persist loyally without feverish expectations.",
        "None",
        "Moderate",
        "Severe"
    },
    {
        "antaraya-9",
        Category::Antaraya,
        "OBSTACLE 9/9",
        "Anavasthitatva (Instability in Practice)",
        "Steadiness, depth, and anchored tranquility.",
        "Did you touch peace or clarity, only to lose it immediately when routine shifted?",
        "Establish an uncompromised daily anchor. Regularity is the mother of mastery.",
        "None",
        "Moderate",
        "Severe"
    },

    // --- 5 SAHABHUVAS (SYMPTOMS) ---
    {
        "sahabhuva-1",
        Category::Sahabhuva,
        "SYMPTOM 1/5",
        "Dukha (Mental Sorrow / Pain)",
        "Compassionate self-awareness and emotional healing.",
        "Did inner grief, mental ache, or emotional suffering cloud your day?",
        "Acknowledge sorrow without identifying as the victim. You are the unaffected witness.",
        "None",
        "Moderate",
        "Severe"
    },
    {
        "sahabhuva-2",
        Category::Sahabhuva,
        "SYMPTOM 2/5",
        "Daurmanasya (Bitterness / Despair)",
        "Cheerfulness, inner equanimity, and benevolent outlook.",
        "Did frustration, bitterness, or cynicism color your attitude toward life or others?",
        "Cultivate Maitri (friendliness) and actively count five simple blessings right now.",
        "None",
        "Moderate",
        "Severe"
    },
    {
        "sahabhuva-3",
        Category::Sahabhuva,
        "SYMPTOM 3/5",
        "Angamejayatva (Restlessness of the Limbs)",
        "Nervous system stability, grounded calm, and physical stillness.",
        "Did you notice nervous fidgeting, involuntary twitching, or physical agitation?",
        "Settle into a stable, comfortable posture (Sthira Sukham Asanam) and breathe deeply.",
        "None",
        "Moderate",
        "Severe"
    },
    {
        "sahabhuva-4",
        Category::Sahabhuva,
        "SYMPTOM 4/5",
        "Shvasha (Irregular Inhalation)",
        "Rhythmic, expansive breath and balanced autonomic tone.",
        "Was inhalation shallow, labored, or hurried due to stress or anxiety?",
        "Consciously soften the chest and take slow, smooth, unforced inhalations into the belly.",
        "None",
        "Moderate",
        "Severe"
    },
    {
        "sahabhuva-5",
        Category::Sahabhuva,
        "SYMPTOM 5/5",
        "Prashvaasha (Irregular Exhalation)",
        "Complete nervous release, surrender, and deep relaxation.",
        "Was exhalation choppy, incomplete, or abruptly forced?",
        "Lengthen your exhalation so it becomes twice as long as inhalation. Let everything go.",
        "None",
        "Moderate",
        "Severe"
    }
};

constexpr int kTotalItems = static_cast<int>(sizeof(kItems) / sizeof(kItems[0]));

}  // namespace sanyam
