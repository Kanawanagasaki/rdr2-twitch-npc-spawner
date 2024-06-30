namespace Rdr2.TwitchNpcSpawner.Entities;

using Rdr2.TwitchNpcSpawner.Client.Shared;
using Microsoft.EntityFrameworkCore;
using System.ComponentModel.DataAnnotations;
using System.ComponentModel.DataAnnotations.Schema;

[Table("rewards"), Index(nameof(UserId)), Index(nameof(TwitchId))]
public class RewardEntity
{
    [Key, DatabaseGenerated(DatabaseGeneratedOption.Identity)]
    public int Id { get; init; }
    public required int UserId { get; init; }
    public required ERewardType Type { get; init; }
    public bool IsCreated { get; set; }
    public string? TwitchId { get; set; }

    public required string Title { get; set; }
    public required int Cost { get; set; }
    public bool IsUserInputRequired { get; set; }
    public string? Prompt { get; set; }
    public string? BackgroundColor { get; set; }

    public string? Extra { get; set; } = null;
}
