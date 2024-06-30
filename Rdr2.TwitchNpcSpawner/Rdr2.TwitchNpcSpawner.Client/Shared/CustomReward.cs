namespace Rdr2.TwitchNpcSpawner.Client.Shared;

public class CustomReward
{
    public required int Id { get; init; }
    public required ERewardType Type { get; init; }
    public bool IsCreated { get; set; }
    public string? Extra { get; set; }

    public TwitchCustomReward? TwitchModel { get; set; }
}
