namespace Rdr2.TwitchNpcSpawner.Client.Shared;

public class Redemption
{
    public required string id { get; init; }
    public required string user_login { get; init; }
    public required string user_id { get; init; }
    public required string user_name { get; init; }
    public required string user_input { get; init; }
    public required string redeemed_at { get; init; }

    public ERewardType reward_type { get; set; }
    public string? extra { get; set; }
}
