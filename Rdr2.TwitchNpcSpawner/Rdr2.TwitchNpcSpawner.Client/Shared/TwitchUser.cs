namespace Rdr2.TwitchNpcSpawner.Client.Shared;

public class TwitchUser
{
    public string? id { get; init; }
    public string? login { get; init; }
    public string? display_name { get; init; }
    public string? type { get; init; }
    public string? broadcaster_type { get; init; }
    public string? description { get; init; }
    public string? profile_image_url { get; init; }
    public string? offline_image_url { get; init; }
    public int view_count { get; init; }
    public string? email { get; init; }
    public string? created_at { get; init; }
}
