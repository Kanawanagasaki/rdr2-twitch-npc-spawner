namespace Rdr2.TwitchNpcSpawner.Client.Shared;

public class CustomRewardReq
{
    public required string title { get; set; }
    public required int cost { get; set; }
    public bool is_user_input_required { get; set; } = false;
    public string? prompt { get; set; }
    public bool is_enabled { get; set; } = true;
    public bool is_paused { get; set; } = false;

    public string? background_color { get; set; }

    public bool is_max_per_stream_enabled { get; set; } = false;
    public int max_per_stream { get; set; } = 9999;

    public bool is_max_per_user_per_stream_enabled { get; set; } = false;
    public int max_per_user_per_stream { get; set; } = 1;

    public bool is_global_cooldown_enabled { get; set; } = false;
    public int global_cooldown_seconds { get; set; } = 900;

    public bool should_redemptions_skip_request_queue { get; set; } = false;

    public string? extra { get; set; } = null;
}
