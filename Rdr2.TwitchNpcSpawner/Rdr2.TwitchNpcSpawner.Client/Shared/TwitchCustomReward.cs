namespace Rdr2.TwitchNpcSpawner.Client.Shared;

public class TwitchCustomReward
{
    public required string id { get; init; }

    public string? broadcaster_id { get; init; }
    public string? broadcaster_login { get; init; }
    public string? broadcaster_name { get; init; }

    public Dictionary<string, string>? image { get; init; }
    public Dictionary<string, string>? default_image { get; init; }

    public string? background_color { get; set; }

    public string? title { get; set; }
    public int cost { get; set; }
    public bool is_enabled { get; set; }
    public bool is_user_input_required { get; set; }
    public string? prompt { get; set; }

    public bool is_paused { get; init; }
    public bool is_in_stock { get; init; }
    public bool should_redemptions_skip_request_queue { get; init; }

    public TwitchCustomRewardMaxPerStreamSetting? max_per_stream_setting { get; init; }
    public TwitchCustomRewardMaxPerUserPerStreamSetting? max_per_user_per_stream_setting { get; init; }
    public TwitchCustomRewardGlobalCooldownSetting? global_cooldown_setting { get; init; }

    public CustomRewardReq ToReq()
        => new()
        {
            title = title ?? "NPC Spawner " + Random.Shared.Next(1, 10_000),
            cost = cost,
            is_user_input_required = is_user_input_required,
            prompt = prompt,
            is_enabled = is_enabled,
            is_paused = is_paused,
            background_color = background_color,
            is_max_per_stream_enabled = max_per_user_per_stream_setting?.is_enabled ?? false,
            max_per_stream = max_per_stream_setting?.max_per_stream ?? 9999,
            is_max_per_user_per_stream_enabled = max_per_user_per_stream_setting?.is_enabled ?? false,
            max_per_user_per_stream = max_per_user_per_stream_setting?.max_per_user_per_stream ?? 1,
            is_global_cooldown_enabled = global_cooldown_setting?.is_enabled ?? true,
            global_cooldown_seconds = global_cooldown_setting?.global_cooldown_seconds ?? 900,
            should_redemptions_skip_request_queue = false
        };
}

// 💀
public record TwitchCustomRewardMaxPerStreamSetting(bool is_enabled, int max_per_stream);
public record TwitchCustomRewardMaxPerUserPerStreamSetting(bool is_enabled, int max_per_user_per_stream);
public record TwitchCustomRewardGlobalCooldownSetting(bool is_enabled, int global_cooldown_seconds);
